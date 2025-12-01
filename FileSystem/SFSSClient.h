// ALL CHAR* OF THE STRUCT MUST BE FREE TO AVOID MEMORY LEAK

#include "FileEntry.h"

struct iOResponse
{
    char* prefix; //Free
    int owner;
    char* path; //Free
    int pathlen;
    char* payload; //Free
    int offset;
};
typedef struct iOResponse IOResponse;

struct dirResponse
{
    char* prefix; //Free
    int owner;
    char* path; //Free
    int pathlen;
};
typedef struct dirResponse DirResponse;

struct listDirResponse
{
    char* prefix; //Free
    int owner;
    char* allFilesNames; //Free
    FileEntry fstlstpositions[40];
    int nrNames;
};
typedef struct listDirResponse ListDirResponse;

int EstabilishConnection(char* hostname, int portN);
int ReadFile(int owner, char* path, int offset, IOResponse* response);
int WriteFile(int owner, char* path, char* content, int offset, IOResponse* response);
int CreateDir(int owner, char* path, char* dirname, DirResponse* response);
int RemoveDir(int owner, char* path, char* dirname, DirResponse* response);
int ListDir(int owner, char* path, ListDirResponse* response);