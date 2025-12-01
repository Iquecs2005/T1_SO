#include "FileOperations.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/dir.h> 

#define INCORRECTPATH -1
#define FAILEDOPERATION -2
#define FUCK -3

extern int alphasort(); 
int file_select(struct direct *entry)
{
    if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
        return 0;
    else
        return 1;
} 

char* GetPath(int owner, char* suffix)
{
    char *buffer;
    
    if ((buffer = getcwd(NULL, 0)) == NULL) 
    {
        perror("Failed to get current directory");
        return NULL;
    } 
    
    char rootFolder[] = "/FileSystem/SFS-root-dir/A";
    char ownerString[] = "0";
    ownerString[0] = '0' + owner;
    
    char* completePath = (char*)malloc(sizeof(char) * (strlen(buffer) + strlen(rootFolder) + strlen(ownerString) + strlen(suffix) + 1));
    
    strcpy(completePath, buffer);
    strcat(completePath, rootFolder);
    strcat(completePath, ownerString);
    strcat(completePath, suffix);
    
    free(buffer); // Free the allocated memory
    
    return completePath;
}

int ReadOperation(int owner, char* path, int pathLen, char* payload, int offset)
{
    char* completePath = GetPath(owner, path);

    FILE* fd = fopen(completePath, "rb");
    free(completePath);
    if (fd == NULL)
    {
        printf("Incorrect File Path\n");
        strcpy(payload, "");
        return INCORRECTPATH;
    }

    fseek(fd, offset, SEEK_SET);
    int nBlocks = fread(payload, 16, 1, fd);
    fclose(fd);
    if (nBlocks == 0)
    {
        printf("Couldnt Read 16 Bytes\n");
        strcpy(payload, "");
        return FAILEDOPERATION;
    }

    return 0;
}

int WriteOperation(int owner, char* path, int pathlen, char* payload, int offset)
{
    char* completePath = GetPath(owner, path);
    printf("Path: %s\n", completePath);
    
    FILE* fd = fopen(completePath, "r+b");
    if (fd == NULL)
    {
        fd = fopen(completePath, "w+b");
        if (fd == NULL) 
        {
            printf("Couldnt Create File\n");
            free(completePath);
            return INCORRECTPATH;
        }
    }
    
    struct stat stat_buffer;
    if (stat(completePath, &stat_buffer) != 0)
    {
        printf("Oh fuck\n");
        return FUCK;
    }
    
    printf("size %d\n", stat_buffer.st_size);
    
    if (offset > stat_buffer.st_size)
    {
        fseek(fd, stat_buffer.st_size, SEEK_SET);
        
        int currentByte = stat_buffer.st_size;
        while (currentByte < offset)
        {
            fwrite("                ", 1, 16, fd);
            currentByte += 16;
        }
    }
    else
    {
        fseek(fd, offset, SEEK_SET);
    }
    
    int nBlocks = 1;
    printf("Len: %d\n", strlen(payload));
    if (payload != NULL && strlen(payload) == 16)
    {
        int nBlocks = fwrite(payload, 1, 16, fd);
        fclose(fd);
        truncate(completePath, offset + 16);
        free(completePath);

        if (nBlocks == 0)
        {
            printf("Couldnt Write 16 Bytes\n");
            payload = NULL;
            return FAILEDOPERATION;
        }
    }
    else
    {
        fclose(fd);
        truncate(completePath, offset);
        free(completePath);
    }

    return 0;
}

int DirCreateOperation(int owner, char* path, int pathlen, char* dirName, int dirlen)
{
    char* suffix = (char*)malloc(sizeof(char) * (strlen(path) + strlen(dirName)));
    strcpy(suffix, path);
    strcat(suffix, dirName);

    char* completePath = GetPath(owner, suffix);
    free(suffix);

    unsigned int mode = S_IRWXU | S_IRWXG | S_IRWXO;
    int status = mkdir(completePath, mode);
    free(completePath);

    if (status == 0)
    {
        printf("Subdirectory Created Correctly\n");
    }
    else
    {
        printf("Subdirectory not created Correctly\n");
        return INCORRECTPATH;
    }

    return 0;
}

int DirRemoveOperation(int owner, char* path, int pathlen, char* dirName, int dirlen)
{
    char* suffix = (char*)malloc(sizeof(char) * (strlen(path) + strlen(dirName)));
    strcpy(suffix, path);
    strcat(suffix, dirName);

    char* completePath = GetPath(owner, suffix);

    int status = RemoveDirectoryElements(completePath);
    status -= rmdir(completePath);
    free(completePath);

    if (status == 0)
    {
        printf("Subdirectory Removed Correctly\n");
        return 0;
    }

    printf("Subdirectory not Removed Correctly\n");
    return INCORRECTPATH;
}

char* DirListOperation(int owner, char* path, int pathlen, FileEntry** filesInfo, int* nFiles)
{
    char* completePath = GetPath(owner, path);
    printf("path: %s\n", completePath);

    struct direct **files;

    int count = scandir( completePath, &files, file_select, alphasort);
    
    int totalLen = 0;
    int currentNFiles = 0;
    for (int i = 1; i < count+1; ++i) 
    {
        totalLen += strlen(files[i-1]->d_name);
        currentNFiles++;
    }

    char* allfilesnames = (char*)malloc(sizeof(char) * totalLen);
    FileEntry* filesEntries = (FileEntry*)malloc(sizeof(FileEntry) * 40);
    allfilesnames[0] = '\0';
    *nFiles = currentNFiles;
    
    int currentLen = -1;
    for (int i = 1; i < count+1; ++i) 
    {
        filesEntries[i-1].startIndex = ++currentLen;
        if (files[i-1]->d_type == DT_DIR)
            filesEntries[i-1].isSubdirectory = 1;
        else
            filesEntries[i-1].isSubdirectory = 0;
        strcat(allfilesnames, files[i-1]->d_name);
        currentLen += strlen(files[i-1]->d_name) - 1;
        filesEntries[i-1].endIndex = currentLen;
        printf("%d, %d, %d\n", filesEntries[i-1].startIndex, filesEntries[i-1].endIndex, filesEntries[i-1].isSubdirectory);
    }
    for (int i = count+1; i < 40; i++)
    {
        filesEntries[i].isSubdirectory = -1;
        filesEntries[i].startIndex = -1;
        filesEntries[i].endIndex = -1;
    }

    *filesInfo = filesEntries;

    free(files);
    free(completePath);

    return allfilesnames;
}

int RemoveDirectoryElements(char* dirName)
{
    int count;
    struct direct **files;
    int dirSize = 0;
    int status = 0;

    count = scandir(dirName, &files, file_select, alphasort);
    if(count <= 0)
    {
        printf("No files in this directiory\n");
        return INCORRECTPATH;
    }

    for(int i = 1; i < count + 1; ++i)
    {
        char filePath[81];
        struct stat fileStats;

        printf("B\n");

        strcpy(filePath, dirName);
        strcat(filePath, "/");
        strcat(filePath, files[i-1]->d_name);

        printf("A\n");

        if(stat(filePath, &fileStats) != 0)
        {
            printf("Error\n");
            return INCORRECTPATH;
        }

        struct direct **tempFiles;
        int tempCount = scandir(filePath, &tempFiles, file_select, alphasort);
        if(tempCount > 0)
        {
            printf("Shouldnt happen\n");
            status += RemoveDirectoryElements(filePath);
            status -= rmdir(filePath);
            free(tempFiles);
        }
        else
        {
            printf("Should happen\n");
            status -= remove(filePath);
        }

    }

    return status;
}
