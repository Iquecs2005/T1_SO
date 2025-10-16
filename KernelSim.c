#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#include "ProcessData.h"

#define OPENMODESYS (O_RDONLY)
#define FIFOSYS "SysCalls"
#define OPENMODEINT (O_RDONLY)
#define FIFOINT "Interuptions"
#define NUM_AP 5

int sharedMemIds[NUM_AP];
ProcessData sharedMemPtr[NUM_AP];
int currentRunningProcess = 0;

void interruptionHandler();
void syscallHandler();

//TODO: criar as filas para D1 e D2
int main(void)
{
    signal(SIGUSR1, interruptionHandler);
    signal(SIGUSR2, syscallHandler);

    for (int i = 0; i < NUM_AP; i++) 
    {
        sharedMemIds[i] = -1; 
    }

    if(pid == 0)
    {
        //descobrir como checar se foi criado ou nao
        //trat
        execlp("./AplicationProcess", sharedMemIds[i]);
    }
    else
    {
        //kernell vai rodar comecando do primeiro entao ele vai checar se foi criado e permitir que vai rodar
        //capturar interrupcoes e chamadas do sistema, chamada do sistema faz
    }
    return 0;
}

int RoundRobinScheduling()
{
    ProcessData* currentProcessData;

    //Stop current process from running
    currentProcessData = sharedMemPtr[currentRunningProcess];
    kill(currentProcessData->pid, SIGSTOP);
    currentProcessData->status = READY;
    
    currentRunningProcess = (currentRunningProcess + 1) % NUM_AP;

    currentProcessData = sharedMemPtr[currentRunningProcess];
    kill(currentProcessData->pid, SIGCONT);
    currentProcessData->status = READY;
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
        execlp("./AplicationProcess", sharedMemIds[i]);
    }
    sharedMemPtr[i]->pid = pid;
    pc->status = RUNNING;

    return sharedMemIds[i];
}

void interruptionHandler()
{
    //ler o tipo de interrupcao da FIFO de interrupcao e rodar o proximo AP na fila de espera do device na interrupcao gerada
    return;
}

void syscallHandler()
{
    //avisa o kernel que o AP precisa ser colocado na fila de espera do device que requisitou
    return;
}