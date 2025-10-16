#include <stdlib.h>
#include <unistd.h>

#include "Syscall.h"

#define OPENMODE (O_WRONLY)
#define FIFO "SysCalls"
#define MAX 5

static int KernelPID;
static int fpFIFO;

int PC = 0;
int Dx;
int Op;

void generateSysCall(int device, int mode);

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

    while (PC < MAX)
    {
        PC++;
        sleep(0.5);
        // generate a random syscall
        if (int d = rand()%100 +1 < 15) 
        { 
            if (d % 2) Dx = D1;
            else Dx= D2;
            if (d % 3 == 1) Op = R;
            else if (d % 3 = 1) Op = W;
            else Op = X;
            generateSysCall(Dx, Op)
        }
        sleep(0.5);
    }
}

void generateSysCall(int device, int operation)
{
    SysCall currentSysCall;
    currentSysCall.device = device;
    currentSysCall.operation = operation;
    write(fpFIFO, &currentSysCall, sizeof(SysCall));
}