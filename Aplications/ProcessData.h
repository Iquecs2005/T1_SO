#define NEW 0
#define READY 1
#define RUNNING 2
#define BLOCKED 3
#define FINISHED 4

#define MAXRESPONSESIZE 1024

typedef struct processData ProcessData;
struct processData
{
    int memoryId;
    int programCounter;
    char doneTransferring;
    char syscallResponse[1024];
};