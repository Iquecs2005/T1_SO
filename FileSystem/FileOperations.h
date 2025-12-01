#include "FileEntry.h"

int ReadOperation(int owner, char* path, int pathLen, char* payload, int offset);
int WriteOperation(int owner, char* path, int pathlen, char* payload, int offset);
int DirCreateOperation(int owner, char* path, int pathlen, char* dirName, int dirlen);
int DirRemoveOperation(int owner, char* path, int pathlen, char* dirName, int dirlen);
char* DirListOperation(int owner, char* path, int pathlen, FileEntry** filesInfo, int* nFiles);