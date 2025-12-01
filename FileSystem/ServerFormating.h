#include "FileEntry.h"

char* RequestFormat1(char* prefix, int owner, char* str1, int int1, char* str2, int int2);
void RequestDeformat1(char* buffer, char** prefix, int* owner, char** str1, int* int1, char** str2, int* int2);
char* RequestFormat2(char* prefix, int owner, char* str1, int int1);
void RequestDeformat2(char* buffer, char** prefix, int* owner, char** str1, int* int1);
char* RequestFormat3(char* prefix, int owner, char* allfilenames, FileEntry* fstlstpositions, int nrnames);
void RequestDeformat3(char* buffer, char** prefix, int* owner, char** allfilenames, FileEntry* fstlstpositions, int* nrnames);
int BufferSize();