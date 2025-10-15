#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include "Interruptions.h"

#define OPENMODE (O_RDONLY | O_NONBLOCK)
#define FIFO "Interuptions"
#define True 1
#define False 0

static int KernelPID;
static int fpFIFO;

void GenerateInterruption(int device);

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        perror("No KernelPID in program call\n");
        return -1;
    }

    sscanf(argv[1], "%d", &KernelPID);

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
    
    Interruption currentInteruption;
    while (access(FIFO, F_OK) == -1)
    {
        sleep(0.5);

        //Generate IRQ0 / Time Slice
        GenerateInterruption(IRQ0);

        if ((rand() % 100) + 1 < 10)
        {
            //Generate IRQ1
            GenerateInterruption(IRQ1);
        }

        if ((rand() % 100) + 1 < 5)
        {
            //Generate IRQ2
            GenerateInterruption(IRQ2);
        }

        kill(KernelPID, SIGUSR1);
    }

    close(fpFIFO);
}

void GenerateInterruption(int device)
{   
    Interruption currentInteruption;
    currentInteruption.device = IRQ2;
    write(fpFIFO, &currentInteruption, sizeof(Interruption));
}