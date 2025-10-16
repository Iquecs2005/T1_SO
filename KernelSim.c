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

void interruptionHandler();
void syscallHandler();

//TODO: criar as filas para D1 e D2
int main(void)
{
    int sharedMemIds[5];
    int* sharedMemPtr[5];

    for (int i = 0; i < NUM_AP; i++) 
    {
        if((pid_t pid = fork() < 0))
        {
            perror("Error creating child");
            exit(1);
        }
        if(pid == 0)
        {
            break;
        }
        sharedMemIds[i] = shmget(IPC_PRIVATE, sizeof(ProcessData), IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
        sharedMemPtr[i] = shmat(sharedMemIds[i], 0, 0);
        ProcessData* pc =  sharedMemPtr[i];
        pc->pid = sharedMemIds[i];
        pc->kernelPid = getpid();
        pc->programCounter = 0;
    }

    if(pid == 0)
    {
        //descobrir como checar se foi criado ou nao
        //trat
        signal(SIGUSR2, syscallHandler);
        execlp("./AplicationProcess", sharedMemIds[i]);
    }
    else
    {
        //kernell vai rodar comecando do primeiro entao ele vai checar se foi criado e permitir que vai rodar
        //capturar interrupcoes e chamadas do sistema, chamada do sistema faz
        signal(SIGUSR1, interruptionHandler);
    return 0;
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