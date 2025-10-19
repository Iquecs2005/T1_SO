#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "Interruptions.h"

#define OPENMODE (O_WRONLY)
#define FIFO "Interuptions"
#define True 1
#define False 0
#define D1PROB 10
#define D2PROB 5

static int KernelPID;
static int fpFIFO;

void GenerateInterruption(int device);
void stopHandler();

int main()
{
    signal(SIGINT, stopHandler);

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
        sleep(1);
        
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
    close(fpFIFO);
    exit(0);
}