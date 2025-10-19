#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "ProcessData.h"
#include "Interruptions.h"
#include "Syscall.h"
#include "Queue.h"

#define OPENMODESYS (O_RDONLY | O_NONBLOCK)
#define FIFOSYS "SysCalls"
#define OPENMODEINT (O_RDONLY | O_NONBLOCK)
#define FIFOINT "Interuptions"
#define NUM_AP 5
#define NUM_DV 2

#define True 1
#define False 0

typedef struct pcb PCB;
struct pcb
{
    int pid;
    int status;
    int programCounter;
};

static int fpSysFifo;
static int fpIntFifo;

static int mainMemoryid;
static int currentRunningProcess = -1;
static int nChild = NUM_AP;
static int interControllerPID = 0;

static char ending = False;

static ProcessData* mainMemory;

static PCB processPCBs[NUM_AP];

static Queue* DevicesQueues[NUM_DV];

int RoundRobinScheduling();
void CreatePCB(int i);
void CreateProcess(int i);
void ContinueCurrentProcess();
void StopCurrentProcess();
void SaveContext();
void LoadContext(int i);
void interruptionHandler();
void syscallHandler();
void sigchildHandler();
void stopHandler();
void CloseKernel();

int main(void)
{
    signal(SIGUSR1, interruptionHandler);
    signal(SIGUSR2, syscallHandler);
    signal(SIGCHLD, sigchildHandler);
    signal(SIGINT, stopHandler);

    for (int i = 0; i < NUM_DV; i++)
    {
        DevicesQueues[i] = CreateQueue();
    }

    mainMemoryid = shmget(IPC_PRIVATE, sizeof(ProcessData), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    mainMemory = shmat(mainMemoryid, 0, 0);

    if (access(FIFOSYS, F_OK) == -1)
    {
        int error;
        if (error = mkfifo (FIFOSYS, S_IRUSR | S_IWUSR) != 0)
        {
            fprintf (stderr, "Erro ao criar FIFO %s %d\n", FIFOSYS, error);
            perror("mkfifo failed");
            return -1;
        }
        printf("Created Sys Fifo\n");
    }
    
    if (access(FIFOINT, F_OK) == -1)
    {
        int error;
        if (error = mkfifo (FIFOINT, S_IRUSR | S_IWUSR) != 0)
        {
            fprintf (stderr, "Erro ao criar FIFO %s %d\n", FIFOINT, error);
            perror("mkfifo failed");
            return -1;
        }
        printf("Created Int Fifo\n");
    }
    
    if ((fpSysFifo = open (FIFOSYS, OPENMODESYS)) < 0)
    {
        fprintf (stderr, "Erro ao abrir a FIFO %s\n", FIFOSYS);
        return -2;
    }
    
    if ((fpIntFifo = open (FIFOINT, OPENMODEINT)) < 0)
    {
        fprintf (stderr, "Erro ao abrir a FIFO %s\n", FIFOINT);
        return -2;
    }
    
    mainMemory->pid = -1;
    for (int i = 0; i < NUM_AP; i++) 
    {
        processPCBs[i].pid = -1; 
    }
    
    //Create InterControllerSim process
    interControllerPID = fork();
    if (interControllerPID == 0)
    {
        execlp("./InterControllerSim", "./InterControllerSim", NULL);
    }
    
    RoundRobinScheduling();
    while (True)
        pause();

    return 0;
}

int RoundRobinScheduling()
{
    currentRunningProcess = (currentRunningProcess + 1) % NUM_AP;
    
    //Search the Run next process in line
    PCB currentProcessData;
    for (int i = 0; i < NUM_AP; i++)
    {   
        currentProcessData = processPCBs[currentRunningProcess];

        if (currentProcessData.pid == -1)
        {
            CreateProcess(currentRunningProcess);
            currentProcessData = processPCBs[currentRunningProcess];
            printf("Creating process %d\n", currentRunningProcess);
            printf("Running process %d\n", currentProcessData.pid);
            printf("PC: %d\n", currentProcessData.programCounter);
            return 1;
        }

        if (currentProcessData.status == READY)
        {
            printf("Running process %d\n", currentProcessData.pid);
            printf("PC: %d\n", currentProcessData.programCounter);
            LoadContext(currentRunningProcess);
            kill(currentProcessData.pid, SIGCONT);
            mainMemory->pid = currentProcessData.pid;
            processPCBs[currentRunningProcess].status = RUNNING;
            return 0;
        }

        currentRunningProcess = (currentRunningProcess + 1) % NUM_AP;
    }

    return -1;
}

void CreatePCB(int i)
{
    if (processPCBs[i].pid != -1)
    {
        return;
    }

    processPCBs[i].status = NEW;
    processPCBs[i].programCounter = 0;
}

void CreateProcess(int i)
{
    CreatePCB(i);
    
    int pid = fork();
    if (pid == 0)
    {
        char number[81];
        sprintf(number, "%d", mainMemoryid);
        execlp("./AplicationProcess", "./AplicationProcess", number, NULL);
    }
    processPCBs[i].pid = pid;
    processPCBs[i].status = RUNNING;
    mainMemory->pid = pid;
    mainMemory->status = RUNNING;
}

void ContinueCurrentProcess()
{
    //Stop current process from running
    printf("continue\n");
    
    PCB currentPCB = processPCBs[currentRunningProcess];
    if (currentPCB.status == READY)
    {
        LoadContext(currentRunningProcess);
        kill(currentPCB.pid, SIGCONT);
        processPCBs[currentRunningProcess].status = RUNNING;
        return;
    }

    fprintf (stderr, "No running Process\n");
}

void StopCurrentProcess()
{
    //Stop current process from running
    if (mainMemory->pid == -1)
        return;

    kill(mainMemory->pid, SIGSTOP);
    SaveContext();
}

void SaveContext()
{
    if (mainMemory->pid == -1)
        return;

    PCB currentProcessData = processPCBs[mainMemory->memoryId];

    //Save
    processPCBs[currentRunningProcess].programCounter = mainMemory->programCounter;
    processPCBs[currentRunningProcess].status = READY;
    mainMemory->pid = -1;
}

void LoadContext(int i)
{
    //Load
    mainMemory->memoryId = i;
    mainMemory->pid = processPCBs[i].pid;
    mainMemory->programCounter = processPCBs[i].programCounter;
}

void interruptionHandler()
{
    //ler o tipo de interrupcao da FIFO de interrupcao e rodar o proximo AP na fila de espera do device na interrupcao gerada
    StopCurrentProcess();

    Interruption interuption;
    interuption.device = -1;
    char interruptions[] = {False, False, False};

    int readStatus = read(fpIntFifo, &interuption, sizeof(Interruption));

    if (readStatus == -1)
        printf("ReadError\n");

    while (readStatus > 0)
    {
        interruptions[interuption.device] = True;
        readStatus = read(fpIntFifo, &interuption, sizeof(Interruption));
    }

    if (interruptions[1])
    {
        printf("D1 Interruption\n");
        int id = pop(DevicesQueues[0]);
        if (id != -1)
        {
            printf("id: %d pid: %d\n", id, processPCBs[id].pid);
            processPCBs[id].status = READY;
        }
    }
    if (interruptions[2])
    {
        printf("D2 Interruption\n");
        int id = pop(DevicesQueues[1]);
        if (id != -1)
        {
            printf("id: %d pid: %d\n", id, processPCBs[id].pid);
            processPCBs[id].status = READY;
        }
    }
    if (interruptions[0])
    {
        printf("ClockInterruption\n");
        RoundRobinScheduling();
    }
    else
        ContinueCurrentProcess();

    return;
}

void syscallHandler()
{
    StopCurrentProcess();
    
    SysCall systemCall;
    read(fpSysFifo, &systemCall, sizeof(SysCall));

    printf("System Call: %d - %d\n", processPCBs[currentRunningProcess].pid, systemCall.device);
    processPCBs[currentRunningProcess].status = BLOCKED;

    Enqueue(DevicesQueues[systemCall.device - 1], currentRunningProcess);
    RoundRobinScheduling();

    return;
}

void sigchildHandler()
{
    if (ending)
        return;

    int status;
    pid_t child_pid = waitpid(-1, &status, WNOHANG);
        
    if (child_pid > 0) 
    {
        SaveContext();
        nChild--;
        printf("Child process %d exited\n", child_pid);
        if (nChild > 0)
        {
            processPCBs[currentRunningProcess].status = FINISHED;
            RoundRobinScheduling();
        }
        else
        {
            CloseKernel();
        }
    }

    return;
}

void stopHandler()
{
    ending = True;
    for (int i = 0; i < NUM_AP; i++)
    {
        kill(processPCBs[i].pid, SIGKILL);
    }
    kill(interControllerPID, SIGKILL);

    CloseKernel();
}

void CloseKernel()
{
    //Close FIFO
    close(fpSysFifo);
    close(fpIntFifo);
    
    //Close Shared Memory
    shmctl(mainMemoryid, IPC_RMID, NULL);
    
    exit(0);
}