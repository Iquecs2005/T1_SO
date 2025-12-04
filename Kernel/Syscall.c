#include "Syscall.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>

#include "../Aplications/ProcessData.h"

#define OPENMODE (O_WRONLY)
#define FIFO "SysCalls"

static int processPid = -1;
static int kernelPID = -1;
static int fpFIFO = -1;
static ProcessData* processData = NULL;

void stopHandler();

void initialize(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror("No PID in program call\n");
        return -1;
    }
    
    sscanf(argv[1], "%d", &processPid);

    signal(SIGINT, stopHandler);

    kernelPID = getppid();

    void* sharedMemPointer = shmat(processPid, NULL, NULL);
    if (sharedMemPointer == -1)
    {
        perror("Couldn't open shared memory");
        exit(1);
    }
    processData = (ProcessData*) sharedMemPointer;

    if (access(FIFO, F_OK) == -1)
    {
        fprintf (stderr, "Erro: FIFO de SystemCalls não pode ser acessada\n");
        return -1;
    }

    if ((fpFIFO = open (FIFO, OPENMODE)) < 0)
    {
        fprintf (stderr, "Erro ao abrir a FIFO %s\n", FIFO);
        return -2;
    }
}

int getPC()
{
    return processData->programCounter;
}

void increasePC()
{
    processData->programCounter++;
}

void sysWrite(char* path, char* payload, int offset)
{
    SysCall currentSysCall;

    processData->doneTransferring = 0;

    currentSysCall.operation = W;
    currentSysCall.id = processData->memoryId;
    strcpy(currentSysCall.path, path);
    strcpy(currentSysCall.payload, payload);
    currentSysCall.offset = offset;

    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);

    while (!processData->doneTransferring);
}

void sysRead(char* path, char* buffer, int offset)
{
    SysCall currentSysCall;

    processData->doneTransferring = 0;
    
    currentSysCall.operation = R;
    currentSysCall.id = processData->memoryId;
    strcpy(currentSysCall.path, path);
    currentSysCall.offset = offset;

    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);

    while (!processData->doneTransferring);

    memcpy(buffer, processData->syscallResponse, 16);
}

char* sysAdd(char* path, char* dirname)
{
    SysCall currentSysCall;

    processData->doneTransferring = 0;

    currentSysCall.operation = A;
    currentSysCall.id = processData->memoryId;
    strcpy(currentSysCall.path, path);
    strcpy(currentSysCall.dirName, dirname);

    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);

    while (!processData->doneTransferring);

    char* newpath = (char*)malloc(sizeof(char) * 256);
    strcpy(newpath, processData->syscallResponse);

    return newpath;
}

int sysRemove(char* path, char* dirname)
{
    SysCall currentSysCall;

    processData->doneTransferring = 0;

    currentSysCall.operation = D;
    currentSysCall.id = processData->memoryId;
    strcpy(currentSysCall.path, path);
    strcpy(currentSysCall.dirName, dirname);

    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);

    while (!processData->doneTransferring);

    int len1;
    memcpy(&len1, processData->syscallResponse, 4);

    return len1;
}

void sysListDir(char* path, char* alldirinfo, FileEntry* fstlstpositions, int* nNames)
{
    SysCall currentSysCall;

    processData->doneTransferring = 0;

    currentSysCall.operation = L;
    currentSysCall.id = processData->memoryId;
    strcpy(currentSysCall.path, path);

    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);

    while (!processData->doneTransferring);

    int len1;
    strcpy(alldirinfo, processData->syscallResponse);
    int index = strlen(alldirinfo); 
    memcpy(fstlstpositions, processData->syscallResponse + index, sizeof(FileEntry) * 40);
    index += sizeof(FileEntry) * 40;
    memcpy(&len1, processData->syscallResponse + index, 4);
}

void stopHandler() 
{

}