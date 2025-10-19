#define NEW 0
#define READY 1
#define RUNNING 2
#define BLOCKED 3
#define FINISHED 4

typedef struct processData ProcessData;
struct processData
{
    int pid;
    int memoryId;
    int status;
    int programCounter;
};