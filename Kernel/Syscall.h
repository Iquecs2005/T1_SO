#define D1 1
#define D2 2
#define R 1
#define W 2
#define A 3
#define D 4
#define L 5

typedef struct syscall SysCall;
struct syscall
{
    int id;
    int device;
    int operation;
    char payload[17];
    int offset;
    char path[81];
    char dirName[81];
};
