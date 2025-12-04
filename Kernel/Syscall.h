#define D1 1
#define D2 2
#define R 1
#define W 2
#define A 3
#define D 4
#define L 5

#include "../FileSystem/FileEntry.h"

typedef struct syscall SysCall;
struct syscall
{
    int id;
    int operation;
    char payload[17];
    int offset;
    char path[81];
    char dirName[81];
};

void initialize(int argc, char *argv[]);
int getPC();
void increasePC();
void sysWrite(char* path, char* payload, int offset);
void sysRead(char* path, char* buffer, int offset);
char* sysAdd(char* path, char* dirname);
int sysRemove(char* path, char* dirname);
void sysListDir(char* path, char* alldirinfo, FileEntry* fstlstpositions, int* nNames);