#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/shm.h>
#include <time.h>

#include "../Kernel/Syscall.h"
#include "ProcessData.h"

#define OPENMODE (O_WRONLY)
#define FIFO "SysCalls"
#define MAX 10
#define SYSCALLPROB 15

static ProcessData* processData;
static int kernelPID;
static int fpFIFO;
static char words[4][17] = {"aaaaaaaaaaaaaaaa", "henriquecarvalho", "joaomiguelfranca", "hollowknightsilk"};
static char* paths[] = {"/alo.txt", "/subdir/atum.txt", "/HollowKnight.txt"};

void generateSysCall(int device, int operation, char* payload, int offset);
void OnExecute();
void stopHandler();


void sleep_ms_nanosleep(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, stopHandler);
    srand(time(NULL));

    if (argc < 2)
    {
        perror("No PID in program call\n");
        return -1;
    }
    
    kernelPID = getppid();
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
        sleep_ms_nanosleep(500);
        // generate a random syscall
        int d;
        if ((d = rand() % 100 + 1) < SYSCALLPROB) 
        { 
            int Dx;
            int Op;  
            char payload[17] = "";
            int offset = 0;          
            //if ((rand() % 100) + 1 < 51) 
            if (rand() % 2 == 0)
            {
                Dx = D1;
                int n = rand() % 2 + 1;
                if (n == 1)
                    Op = R;
                else
                {
                    Op = W;
                    int word = rand() % 4;
                    strcpy(payload, words[word]);
                }
                int off = rand() % 7;
                offset = off * 16;
            }
            else 
            {
                Dx = D2;
                int n = rand() % 3 + 1;
                if (n == 1)
                    Op = A;
                else if (n == 2) 
                    Op = D;
                else if (n == 3)
                    Op = L;
            }
            processData->programCounter++;
            generateSysCall(Dx, Op, payload, offset);
        }
        else
        {
            processData->programCounter++;
        }
        sleep_ms_nanosleep(500);
    }
}

void generateSysCall(int device, int operation, char* payload, int offset)
{
    SysCall currentSysCall;
    currentSysCall.id = processData->memoryId;
    currentSysCall.device = device;
    currentSysCall.operation = operation;
    strcpy(currentSysCall.payload, payload);
    strcpy(currentSysCall.path, paths[rand() % 3]);
    currentSysCall.offset = offset;
    write(fpFIFO, &currentSysCall, sizeof(SysCall));
    kill(kernelPID, SIGUSR2);
    return;
}

void stopHandler() 
{

}