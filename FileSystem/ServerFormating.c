#include "ServerFormating.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFERSIZE 1024

int BufferToStr(char* buffer, char** str);
int StrToBuffer(char* str, char* buffer);
int IntToBuffer(int intVariable, char* buffer);
int BufferToInt(unsigned char* buffer, int* value);
int FileIndexToBuffer(FileEntry fileEntry, char* buffer);
int BufferToFileIndex(FileEntry* fileEntry, unsigned char* buffer);

char* RequestFormat1(char* prefix, int owner, char* str1, int int1, char* str2, int int2)
{
    char* buffer = (char*)malloc(BUFFERSIZE);

    int currentIndex = 0;
    currentIndex += StrToBuffer(prefix, buffer + currentIndex);
    currentIndex += IntToBuffer(owner, buffer + currentIndex);
    currentIndex += StrToBuffer(str1, buffer + currentIndex);
    currentIndex += IntToBuffer(int1, buffer + currentIndex);
    currentIndex += StrToBuffer(str2, buffer + currentIndex);
    currentIndex += IntToBuffer(int2, buffer + currentIndex);

    return buffer;
}

void RequestDeformat1(char* buffer, char** prefix, int* owner, char** str1, int* int1, char** str2, int* int2)
{
    int currentIndex = 0;

    currentIndex += BufferToStr(buffer + currentIndex, prefix);
    currentIndex += BufferToInt(buffer + currentIndex, owner);
    currentIndex += BufferToStr(buffer + currentIndex, str1);
    currentIndex += BufferToInt(buffer + currentIndex, int1);
    currentIndex += BufferToStr(buffer + currentIndex, str2);
    currentIndex += BufferToInt(buffer + currentIndex, int2);
}

char* RequestFormat2(char* prefix, int owner, char* str1, int int1)
{
    char* buffer = (char*)malloc(BUFFERSIZE);

    int currentIndex = 0;
    currentIndex += StrToBuffer(prefix, buffer + currentIndex);
    currentIndex += IntToBuffer(owner, buffer + currentIndex);
    currentIndex += StrToBuffer(str1, buffer + currentIndex);
    currentIndex += IntToBuffer(int1, buffer + currentIndex);

    return buffer;
}

void RequestDeformat2(char* buffer, char** prefix, int* owner, char** str1, int* int1)
{
    int currentIndex = 0;

    currentIndex += BufferToStr(buffer + currentIndex, prefix);
    currentIndex += BufferToInt(buffer + currentIndex, owner);
    currentIndex += BufferToStr(buffer + currentIndex, str1);
    currentIndex += BufferToInt(buffer + currentIndex, int1);
}

char* RequestFormat3(char* prefix, int owner, char* allfilenames, FileEntry* fstlstpositions, int nrnames)
{
    char* buffer = (char*)malloc(BUFFERSIZE);

    int currentIndex = 0;
    currentIndex += StrToBuffer(prefix, buffer + currentIndex);
    currentIndex += IntToBuffer(owner, buffer + currentIndex);
    currentIndex += StrToBuffer(allfilenames, buffer + currentIndex);
    for (int i = 0; i < 40; i++)
    {
        currentIndex += FileIndexToBuffer(fstlstpositions[i], buffer + currentIndex);
    }
    currentIndex += IntToBuffer(nrnames, buffer + currentIndex);

    return buffer;
}

void RequestDeformat3(char* buffer, char** prefix, int* owner, char** allfilenames, FileEntry* fstlstpositions, int* nrnames)
{
    int currentIndex = 0;

    currentIndex += BufferToStr(buffer + currentIndex, prefix);
    currentIndex += BufferToInt(buffer + currentIndex, owner);
    currentIndex += BufferToStr(buffer + currentIndex, allfilenames);
    for (int i = 0; i < 40; i++)
    {
        currentIndex += BufferToFileIndex(fstlstpositions + i, buffer + currentIndex);
    }
    currentIndex += BufferToInt(buffer + currentIndex, nrnames);
}

int BufferSize()
{
    return BUFFERSIZE;
}

int StrToBuffer(char* str, char* buffer)
{
    int i = 0;
    while (str[i] != '\0')
    {
        buffer[i] = str[i];
        i++;
    }
    buffer[i] = '\0';
    i++;

    return i;
}

int BufferToStr(char* buffer, char** str)
{
    int len = strlen(buffer);
    char* value = (char*)malloc(sizeof(char) * len);
    strcpy(value, buffer);
    *str = value;

    return len + 1;
}

int IntToBuffer(int intVariable, char* buffer)
{
    for (int i = 0; i < 4; i++)
    {
        buffer[i] = (intVariable >> (8 * i)) & 0xff;
    }
    return 4;
}

int BufferToInt(unsigned char* buffer, int* value)
{
    int currentValue = 0;
    for (int i = 0; i < 4; i++)
    {
        currentValue += (unsigned int)(buffer[i] << (8 * i));
    }

    *value = currentValue;
    return 4;
}

int FileIndexToBuffer(FileEntry fileEntry, char* buffer)
{
    int currentIndex = 0;

    currentIndex += IntToBuffer(fileEntry.startIndex, buffer + currentIndex);
    currentIndex += IntToBuffer(fileEntry.endIndex, buffer + currentIndex);
    currentIndex += IntToBuffer(fileEntry.isSubdirectory, buffer + currentIndex);

    return currentIndex;
}

int BufferToFileIndex(FileEntry* fileEntry, unsigned char* buffer)
{
    int currentIndex = 0;
    
    currentIndex += BufferToInt(buffer + currentIndex, &(fileEntry->startIndex));
    currentIndex += BufferToInt(buffer + currentIndex, &(fileEntry->endIndex));
    currentIndex += BufferToInt(buffer + currentIndex, &(fileEntry->isSubdirectory));

    return currentIndex;
}