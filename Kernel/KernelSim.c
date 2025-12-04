#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include "../Aplications/ProcessData.h"
#include "../InterFolder/Interruptions.h"
#include "../FileSystem/SFSSClient.h"
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

enum requestType 
{
    requestRead,
    requestWrite,
    requestCreate,
    requestDelete,
    requestListAll
};

typedef struct reply REP;
struct reply
{
    enum requestType type;
    union responseData
    {
        IOResponse io;
        DirResponse dir;
        ListDirResponse list;
    } response;
    int received;
};

typedef struct openFile OF;
struct openFile
{
    char filePath[257];
    int currentPos;
};

typedef struct processState PS;
struct processState
{
    OF openFiles[100];
    int filesCount = 0;
    REP reply;
};

typedef struct pcb PCB;
struct pcb
{
    int pid;
    int status;
    char doneTransferring;
    char syscallResponse[1024];
    int programCounter;
    int device;
    int operation;
    PS processState;
    int nRequest[NUM_DV];
};

static int fpSysFifo;
static int fpIntFifo;

static int mainMemoryid;
static int currentRunningPid;
static int currentRunningProcess = -1;
static int nChild = NUM_AP;
static int sfssPID = 0;
static int interControllerPID = 0;

static char paused = False;

static ProcessData* mainMemory;

static PCB processPCBs[NUM_AP];

static Queue* DevicesQueues[NUM_DV];

int CreatePipe(char* fifoName, int openMode);
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
void continueHandler();

int main(void)
{
    signal(SIGUSR1, interruptionHandler);
    signal(SIGUSR2, syscallHandler);
    signal(SIGCHLD, sigchildHandler);
    signal(SIGINT, stopHandler);
    signal(SIGCONT, continueHandler);

    for (int i = 0; i < NUM_DV; i++)
    {
        DevicesQueues[i] = CreateQueue();
    }

    mainMemoryid = shmget(IPC_PRIVATE, sizeof(ProcessData), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    mainMemory = shmat(mainMemoryid, 0, 0);
    
    fpSysFifo = CreatePipe(FIFOSYS, OPENMODESYS);
    fpIntFifo = CreatePipe(FIFOINT, OPENMODEINT);
    
    currentRunningPid = -1;
    for (int i = 0; i < NUM_AP; i++) 
    {
        processPCBs[i].pid = -1; 
    }
    
    //Create SFSS process
    sfssPID = fork();
    if (sfssPID == 0)
    {
        execlp("./FileSys", "./FileSys", "3999", NULL);
    }
    EstabilishConnection("Localhost", 3999);

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

int CreatePipe(char* fifoName, int openMode)
{
    if (access(fifoName, F_OK) == -1)
    {
        int error;
        if (error = mkfifo (fifoName, S_IRUSR | S_IWUSR) != 0)
        {
            fprintf (stderr, "Erro ao criar FIFO %s %d\n", fifoName, error);
            perror("mkfifo failed");
            return -1;
        }
    }

    int pipeId = open (fifoName, openMode);

    if (pipeId < 0)
    {
        fprintf (stderr, "Erro ao abrir a FIFO %s\n", fifoName);
        return -2;
    }

    return pipeId;
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
            LoadContext(currentRunningProcess);
            printf("Running process %d\n", currentProcessData.pid);
            printf("PC: %d\n", currentProcessData.programCounter);
            kill(currentProcessData.pid, SIGCONT);
            currentRunningPid = currentProcessData.pid;
            processPCBs[currentRunningProcess].status = RUNNING;
            processPCBs[currentRunningProcess].processState.reply.received = 1;
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
    for (int j = 0; j < NUM_DV; j++)
    {
        processPCBs[i].nRequest[j] = 0;
    }
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
    processPCBs[i].doneTransferring = 0;
    LoadContext(i);
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
        //chamar a funcao de reply
        processPCBs[currentRunningProcess].status = RUNNING;
        return;
    }

    fprintf (stderr, "No running Process\n");
}

void StopCurrentProcess()
{
    //Stop current process from running
    if (currentRunningPid == -1)
        return;

    kill(currentRunningPid, SIGSTOP);
    SaveContext();
}

void SaveContext()
{
    if (currentRunningPid == -1)
        return;

    PCB currentProcessData = processPCBs[mainMemory->memoryId];

    //Save
    processPCBs[currentRunningProcess].programCounter = mainMemory->programCounter;
    processPCBs[currentRunningProcess].status = READY;
    processPCBs[currentRunningProcess].doneTransferring = mainMemory->doneTransferring;
    memcpy(processPCBs->syscallResponse, mainMemory->syscallResponse, sizeof(char) * 1024);
    currentRunningPid = -1;
}

void LoadContext(int i)
{
    //Load
    mainMemory->memoryId = i;
    mainMemory->programCounter = processPCBs[i].programCounter;
    mainMemory->doneTransferring = processPCBs[i].doneTransferring;
    memcpy(mainMemory->syscallResponse, processPCBs->syscallResponse, sizeof(char) * 1024);
    currentRunningPid = processPCBs[i].pid;
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
    {
        printf("ReadError\n");
        printf("Read status %d\n", readStatus);
        return;
    }


    while (readStatus > 0)
    {
        interruptions[interuption.device] = True;
        readStatus = read(fpIntFifo, &interuption, sizeof(Interruption));
    }

    if (interruptions[1])
    {
        //file operation
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
        //directory operation
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

    printf("System Call: %d - %d\n", processPCBs[currentRunningProcess].pid, systemCall.operation);
    processPCBs[currentRunningProcess].status = BLOCKED;
    processPCBs[currentRunningProcess].operation = systemCall.operation;
    
    int device = 0;
    switch (systemCall.operation)
    {
    case R:
        device = 1;
        processPCBs[currentRunningProcess].processState.reply.type = requestRead;
        IOResponse* readResponse = &processPCBs[currentRunningProcess].processState.reply.response.io;
        processPCBs[currentRunningProcess].processState.reply.received = 0;
        int fileID = getFile(systemCall.path);
        int offset = systemCall.offset;
        if(fileID >= 0)
        {
            offset += processPCBs[currentRunningProcess].processState.openFiles[fileID].currentPos;
            processPCBs[currentRunningProcess].processState.openFiles[fileID].currentPos += 16;
        }
        ReadFile(currentRunningProcess+1, systemCall.path, offset, readResponse);
        if(readResponse >= 0 && fileID < 0)
        {
            fileID = processPCBs[currentRunningProcess].processState.filesCount;
            strcpy(processPCBs[currentRunningProcess].processState.openFiles[fileID].filePath, systemCall.path);
            processPCBs[currentRunningProcess].processState.openFiles[fileID].currentPos += 16;
            processPCBs[currentRunningProcess].processState.filesCount++;
        }
        printf("Results\n");
        printf("%s, %d, %s, %d, %s, %d\n", readResponse->prefix, readResponse->owner, readResponse->path, readResponse->pathlen, readResponse->payload, readResponse->offset);

        memcpy(processPCBs[currentRunningProcess].syscallResponse, readResponse->payload, 16);

        free(readResponse->prefix);
        free(readResponse->path);
        free(readResponse->payload);
        break;
    case W:
        device = 1;
        processPCBs[currentRunningProcess].processState.reply.type = requestWrite;
        IOResponse* writeResponse = &processPCBs[currentRunningProcess].processState.reply.response.io;
        processPCBs[currentRunningProcess].processState.reply.received = 0;
        WriteFile(currentRunningProcess+1, systemCall.path, systemCall.payload, systemCall.offset, &writeResponse);
        if(getFile(systemCall.path) < 0)
        {
            int fileID = processPCBs[currentRunningProcess].processState.filesCount;
            strcpy(processPCBs[currentRunningProcess].processState.openFiles[fileID].filePath, systemCall.path);
            processPCBs[currentRunningProcess].processState.filesCount++;
        }

        printf("Results\n");
        printf("%s, %d, %s, %d, %s, %d\n", writeResponse->prefix, writeResponse->owner, writeResponse->path, writeResponse->pathlen, writeResponse->payload, writeResponse->offset);

        free(writeResponse->prefix);
        free(writeResponse->path);
        free(writeResponse->payload);
        break;
    case A:
        device = 2;
        processPCBs[currentRunningProcess].processState.reply.type = requestCreate;
        DirResponse* addResponse = &processPCBs[currentRunningProcess].processState.reply.response.dir;
        processPCBs[currentRunningProcess].processState.reply.received = 0;
        CreateDir(currentRunningProcess+1, systemCall.path, systemCall.dirName, &addResponse);
        printf("Results\n");
        printf("%s, %d, %s, %d\n", addResponse->prefix, addResponse->owner, addResponse->path, addResponse->pathlen);

        processPCBs[currentRunningProcess].syscalladdResponse[0] = '\0';
        strcpy(processPCBs[currentRunningProcess].syscallResponse, addResponse->path);

        free(addResponse->prefix);
        free(addResponse->path);
        break;
    case D:
        device = 2;
        processPCBs[currentRunningProcess].processState.reply.type = requestDelete;
        DirResponse* removeResponse = &processPCBs[currentRunningProcess].processState.reply.response.dir;
        processPCBs[currentRunningProcess].processState.reply.received = 0;
        RemoveDir(currentRunningProcess+1, systemCall.path, systemCall.dirName, &removeResponse);
        printf("Results\n");
        printf("%s, %d, %s, %d\n", removeResponse->prefix, removeResponse->owner, removeResponse->path, removeResponse->pathlen);
        
        memcpy(processPCBs[currentRunningProcess].syscallResponse, &(removeResponse->pathlen), 4);
        
        printf("before Segfault\n");
        free(removeResponse->prefix);
        free(removeResponse->path);
        break;
    case L:
        device = 2;
        processPCBs[currentRunningProcess].processState.reply.type = requestListAll;
        DirResponse* listResponse = &processPCBs[currentRunningProcess].processState.reply.response.dir;
        processPCBs[currentRunningProcess].processState.reply.received = 0;
        ListDir(currentRunningProcess+1, systemCall.path, &listResponse);
        printf("Results\n");
        printf("%s %d %s %d\n", listResponse->prefix, listResponse->owner, listResponse->allFilesNames, listResponse->nrNames);
        for (int i = 0; i < listResponse->nrNames; i++)
        {
            printf("%d, %d, %d\n", response->fstlstpositions[i].startIndex, response->fstlstpositions[i].endIndex, response->fstlstpositions[i].isSubdirectory);
        }

        int len1;
        processPCBs[currentRunningProcess].syscallResponse[0] = '\0';
        strcpy(processPCBs[currentRunningProcess].syscallResponse, listResponse->allFilesNames);
        int index = strlen(processPCBs[currentRunningProcess].syscallResponse); 
        memcpy(processPCBs[currentRunningProcess].syscallResponse + index, listResponse->fstlstpositions, sizeof(FileEntry) * 40);
        index += sizeof(FileEntry) * 40;
        memcpy(processPCBs[currentRunningProcess].syscallResponse + index, &(listResponse->nrNames), 4);

        free(listResponse->prefix);
        free(listResponse->allFilesNames);
        break;
    }

    processPCBs[currentRunningProcess].device = device;
    processPCBs[currentRunningProcess].nRequest[device - 1]++;
    
    Enqueue(DevicesQueues[device - 1], currentRunningProcess);

    for (int i = 0; i < NUM_DV; i++)
    {
        printf("Device %d: \n", i + 1);
        printQueue(DevicesQueues[i]);
    }

    RoundRobinScheduling();

    return;
}

void sigchildHandler()
{
    if (paused)
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
            exit(0);
        }
    }

    return;
}

void stopHandler()
{
    paused = True;

    kill(interControllerPID, SIGSTOP);

    if (currentRunningPid != -1)
        kill(currentRunningPid, SIGSTOP);

    printf("\n\nProcess status:\n\n");
    for (int i = 0; i < NUM_AP; i++)
    {
        printf("Process %d\n", i);
        char* message;
        switch (processPCBs[i].status)
        {
        case NEW:
            message = "New";
            break;
        case RUNNING:
            message = "Running";
            break;
        case READY:
            message = "Ready";
            break;
        case BLOCKED:
            message = "Blocked";
            break;
        case FINISHED:
            message = "Finished";
            break;
        }

        printf("Status: %s\n", message);
        printf("Program Counter: %d\n", processPCBs[i].programCounter);

        if (processPCBs[i].status == BLOCKED)
        {
            switch (processPCBs[i].device)
            {
            case D1:
                message = "D1";
                break;
            case D2:
                message = "D2";
                break;
            }
            printf("Device: %s\n", message);

            switch (processPCBs[i].operation)
            {
            case R:
                message = "Read";
                break;
            case W:
                message = "Write";
                break;
            case A:
                message = "Add";
                break;
            case D:
                message = "Delete";
                break;
            case L:
                message = "List";
                break;
            }

            printf("Operation: %s\n", message);
        }

        printf("Open files:\n");
        for (int j = 0; j < processPCBs[i].processState.filesCount; j++)
        {
            printf("file: %s, position: %d\n", processPCBs[i].processState.openFiles[j].filePath, processPCBs[i].processState.openFiles[j].currentPos);
        }

        char* call;
        switch (processPCBs[i].processState.reply.type)
        {
        case requestRead:
            call = "Read";
            break;
        case requestWrite:
            call = "Write";
            break;
        case requestCreate:
            call = "Add";
            break;
        case requestDelete:
            call = "Delete";
            break;
        case requestListAll:
            call = "List";
            break;
        }
        printf("Call %s received: %d\n", call, processPCBs[i].processState.reply.received);

        for (int j = 0; j < NUM_DV; j++)
        {
            printf("D%d - %d\n", j + 1, processPCBs[i].nRequest[j]);
        }   

        printf("\n");
    }

    return;
}

void continueHandler()
{
    paused = False;

    kill(interControllerPID, SIGCONT);

    if (currentRunningPid != -1)
        kill(currentRunningPid, SIGCONT);

    return;
}

int getFile(char* filePath)
{
    for(int i = 0; i < processPCBs[currentRunningProcess]; ++i)
    {
        if(strcmp(processPCBs[currentRunningProcess].processState.openFiles[i].filePath, filePath) == 0)
        {
            return i;
        }
    }
    return -1;
}