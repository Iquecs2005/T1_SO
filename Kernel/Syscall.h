#define D1 1
#define D2 2
#define R 1
#define W 2
#define X 3

typedef struct syscall SysCall;
struct syscall
{
    int id;
    int device;
    int operation;
};
