#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include "Interruptions.h"

#define OPENMODE (O_WRONLY)
#define FIFO "Interuptions"
#define D1PROB 20
#define D2PROB 1

static int KernelPID;
static int fpFIFO;

void GenerateInterruption(int device);
void stopHandler();

void sleep_ms_nanosleep(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    nanosleep(&ts, NULL);
}

int main()
{
    signal(SIGINT, stopHandler);
    srand(time(NULL));

    KernelPID = getppid();
    
    if (access(FIFO, F_OK) == -1)
    {
        int error;
        if (error = mkfifo (FIFO, S_IRUSR | S_IWUSR) != 0)
        {
            fprintf (stderr, "Erro ao criar FIFO %s %d\n", FIFO, error);
            perror("mkfifo failed");
            return -1;
        }
    }
    
    if ((fpFIFO = open (FIFO, OPENMODE)) < 0)
    {
        fprintf (stderr, "Erro ao abrir a FIFO %s\n", FIFO);
        return -2;
    }
    
    while (access(FIFO, F_OK) != -1)
    {
        sleep_ms_nanosleep(500);
        
        //Generate IRQ0 / Time Slice
        GenerateInterruption(IRQ0);
        
        if ((rand() % 100) + 1 < D1PROB)
        {
            //Generate IRQ1
            GenerateInterruption(IRQ1);
        }
        
        if ((rand() % 100) + 1 < D2PROB)
        {
            //Generate IRQ2
            GenerateInterruption(IRQ2);
        }
        
        kill(KernelPID, SIGUSR1);
    }
}

void GenerateInterruption(int device)
{   
    Interruption currentInteruption;
    currentInteruption.device = device;
    if (write(fpFIFO, &currentInteruption, sizeof(Interruption)) < 0)
    {
        printf("Write error\n");
    }
}

void stopHandler() 
{
    
}