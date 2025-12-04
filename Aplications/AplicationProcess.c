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

#define MAX 10
#define SYSCALLPROB 15

#define PAYLOADSIZE 4
#define PATHSIZE 3
#define DIRSIZE 3
#define DIRNAMESSIZE 3

static char* words[] = {"aaaaaaaaaaaaaaaa", "henriquecarvalho", "joaomiguelfranca", "hollowknightsilk"};
static char* paths[] = {"/alo.txt", "/subdir/atum.txt", "/HollowKnight.txt"};
static char* dirs[] = {"/", "/", "/dir1"};
static char* dirNames[] = {"dir1", "subdir2", "subdir3"};

void sleep_ms_nanosleep(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;

    nanosleep(&ts, NULL);
}

int main(int argc, char *argv[])
{
    initialize(argc, argv);

    srand(time(NULL));

    while (getPC() < MAX)
    {
        sleep_ms_nanosleep(500);
        // generate a random syscall
        int d;
        if ((d = rand() % 100 + 1) < SYSCALLPROB) 
        { 
            int value = rand() % 5;

            switch (value)
            {
            case 0:
                sysWrite(paths[rand() % PATHSIZE], words[rand() % PAYLOADSIZE], 16 * (rand() % 10));
                break;
            case 1:
                char buffer[17];
                sysRead(paths[rand() % PATHSIZE], &buffer, 16 * (rand() % 10));
                break;
            case 2:
                sysAdd(dirs[rand() % DIRSIZE], dirNames[rand() % DIRNAMESSIZE]);
                break;
            case 3:
                sysRemove(dirs[rand() % DIRSIZE], dirNames[rand() % DIRNAMESSIZE]);
                break;
            case 4:
                char dirNames[256];
                FileEntry files[40];
                int nNames;
                sysListDir(dirs[rand() % DIRSIZE], dirNames, files, &nNames);
                break;
            }
        }
        increasePC();

        sleep_ms_nanosleep(500);
    }
}