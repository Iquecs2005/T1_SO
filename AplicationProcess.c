#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>

#include "Syscall.h"
#include "ProcessData.h"

#define OPENMODE (O_WRONLY)
#define FIFO "SysCalls"
#define MAX 2

static ProcessData* processData;
static int fpFIFO;

void generateSysCall(int device, int mode);
void OnExecute();

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror("No PID in program call\n");
        return -1;
    }

    int processPid;
    sscanf(argv[1], "%d", &processPid);

    //Anexa a memoria compartilhada criada pelo nucleo contendo as informações do processo
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

    while (processData->programCounter < MAX)
    {
        printf("%d - %d\n", getpid(), processData->programCounter);
        sleep(1);
        // generate a random syscall
        int d;
        if (d = rand()%100 +1 < 15) 
        { 
            int Dx;
            int Op;
            if (d % 2) Dx = D1;
            else Dx= D2;
            if (d % 3 == 1) Op = R;
            else if (d % 3 == 2) Op = W;
            else Op = X;
            processData->programCounter++;
            generateSysCall(Dx, Op);
        }
        else
        {
            processData->programCounter++;
        }
    }
}

void generateSysCall(int device, int operation)
{
    SysCall currentSysCall;
    currentSysCall.id = processData->memoryId;
    currentSysCall.device = device;
    currentSysCall.operation = operation;
    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(processData->kernelPid, SIGUSR2);
    pause();
}