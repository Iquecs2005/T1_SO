#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>

#include "ProcessData.h"
#include "Interruptions.h"
#include "Syscall.h"

#define OPENMODESYS (O_RDONLY | O_NONBLOCK)
#define FIFOSYS "SysCalls"
#define OPENMODEINT (O_RDONLY | O_NONBLOCK)
#define FIFOINT "Interuptions"
#define NUM_AP 5

#define True 1
#define False 0

static int fpSysFifo;
static int fpIntFifo;

int sharedMemIds[NUM_AP];
ProcessData* sharedMemPtr[NUM_AP];
int currentRunningProcess = 0;

int RoundRobinScheduling();
int AlocateProcessMemory(int i);
int CreateProcess(int i);
void ContinueCurrentProcess();
void StopCurrentProcess();
void interruptionHandler();
void syscallHandler();

//TODO: criar as filas para D1 e D2
int main(void)
{
    signal(SIGUSR1, interruptionHandler);
    signal(SIGUSR2, syscallHandler);

    if (access(FIFOSYS, F_OK) == -1)
    {
        int error;
        if (error = mkfifo (FIFOSYS, S_IRUSR | S_IWUSR) != 0)
        {
            fprintf (stderr, "Erro ao criar FIFO %s %d\n", FIFOSYS, error);
            perror("mkfifo failed");
            return -1;
        }
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

    for (int i = 0; i < NUM_AP; i++) 
    {
        sharedMemIds[i] = -1; 
    }

    //Create InterControllerSim process
    if (fork() == 0)
    {
        char number[81];
        sprintf(number, "%d", getpid());
        execlp("./InterControllerSim", number, NULL);
    }

    RoundRobinScheduling();
    pause();

    close(fpSysFifo);
    close(fpIntFifo);

    return 0;
}

int RoundRobinScheduling()
{
    StopCurrentProcess();

    //Search the Run next process in line
    ProcessData* currentProcessData;
    for (int i = 0; i < NUM_AP; i++)
    {   
        if (sharedMemIds[currentRunningProcess] == -1)
        {
            CreateProcess(currentRunningProcess);
            return 1;
        }

        currentProcessData = sharedMemPtr[currentRunningProcess];
        
        if (currentProcessData->status == READY)
        {
            kill(currentProcessData->pid, SIGCONT);
            currentProcessData->status = RUNNING;
            return 0;
        }

        currentRunningProcess = (currentRunningProcess + 1) % NUM_AP;
    }

    return -1;
}

int AlocateProcessMemory(int i)
{
    if (sharedMemIds[i] == -1)
    {
        return -1;
    }

    sharedMemIds[i] = shmget(IPC_PRIVATE, sizeof(ProcessData), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    sharedMemPtr[i] = shmat(sharedMemIds[i], 0, 0);
    ProcessData* pc =  sharedMemPtr[i];
    pc->pid = -1;
    pc->memoryId = sharedMemIds[i];
    pc->kernelPid = getpid();
    pc->status = NEW;
    pc->programCounter = 0;

    return sharedMemIds[i];
}

int CreateProcess(int i)
{
    AlocateProcessMemory(i);

    pid_t pid = fork();
    if (pid == 0)
    {
        char number[81];
        sprintf(number, "%d", sharedMemIds[i]);
        execlp("./AplicationProcess", number, NULL);
    }
    sharedMemPtr[i]->pid = pid;
    sharedMemPtr[i]->status = RUNNING;

    return sharedMemIds[i];
}

void ContinueCurrentProcess()
{
    //Stop current process from running
    ProcessData* currentProcessData;
    currentProcessData = sharedMemPtr[currentRunningProcess];

    if (currentProcessData->status == READY)
    {
        kill(currentProcessData->pid, SIGCONT);
        currentProcessData->status = RUNNING;
        return;
    }

    fprintf (stderr, "No running Process\n");
}

void StopCurrentProcess()
{
    //Stop current process from running
    ProcessData* currentProcessData;
    if (sharedMemIds[currentRunningProcess] != -1)
    {
        currentProcessData = sharedMemPtr[currentRunningProcess];
        if (currentProcessData->status == RUNNING)
        {
            kill(currentProcessData->pid, SIGSTOP);
            currentProcessData->status = READY;
        }
        
        currentRunningProcess = (currentRunningProcess + 1) % NUM_AP;
    }
}

void interruptionHandler()
{
    //ler o tipo de interrupcao da FIFO de interrupcao e rodar o proximo AP na fila de espera do device na interrupcao gerada
    StopCurrentProcess();

    Interruption interuption;
    char clockInterruption;
    while (read(fpIntFifo, &interuption, sizeof(Interruption)) > 0)
    {
        if (interuption.device == IRQ0)
            clockInterruption = True;
    }

    if (clockInterruption)
        RoundRobinScheduling();
    else
        ContinueCurrentProcess();

    return;
}

void syscallHandler()
{
    //avisa o kernel que o AP precisa ser colocado na fila de espera do device que requisitou
    return;
}