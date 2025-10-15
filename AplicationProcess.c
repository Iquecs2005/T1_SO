#include <stdlib.h>
#include <unistd.h>

#include "Syscall.h"

#define MAX 5

int PC = 0;
int Dx;
int Op;

int main(void)
{
    while (PC < MAX)
    {
        PC++;
        sleep(0.5);
        // generate a random syscall
        if (int d = rand()%100 +1 < 15) 
        { 
        if (d % 2) Dx = D1
        else Dx= D2;
        if (d % 3 == 1) Op = R
        else if (d % 3 = 1) Op = W
        else Op = X;
        // generate syscall (Dx, Op)
        }
        sleep(0.5);
    }
}