#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "FileSystem/ServerFormating.h"

int sockfd, portno, n;
int serverlen;
struct sockaddr_in serveraddr;
struct hostent *server;
char *hostname;

void error(char *msg) 
{
    perror(msg);
    exit(0);
}

// EXAMPLE
// int main(int argc, char **argv) 
// {
//     /* check command line arguments */
//     if (argc != 3) {
//        fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
//        exit(0);
//     }
//     hostname = argv[1];
//     portno = atoi(argv[2]);

//     /* socket: create the socket */
//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) 
//         error("ERROR opening socket");

//     /* gethostbyname: get the server's DNS entry */
//     server = gethostbyname(hostname);
//     if (server == NULL) {
//         fprintf(stderr,"ERROR, no such host as %s\n", hostname);
//         exit(0);
//     }

//     /* build the server's Internet address */
//     bzero((char *) &serveraddr, sizeof(serveraddr));
//     serveraddr.sin_family = AF_INET;
//     bcopy((char *)server->h_addr, 
// 	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
//     serveraddr.sin_port = htons(portno);

//     /* get a message from the user */
//     //bzero(buf, BUFSIZE);
//     //printf("Please enter msg: ");
//     //fgets(buf, BUFSIZE, stdin);
    
//     char* buf = RequestFormat2("DL-REQ", 2, "/", strlen("/"));
    
//     //printf("%s\n", buf);
    
//     /* send the message to the server */
//     serverlen = sizeof(serveraddr);
//     n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
//     if (n < 0) 
//     error("ERROR in sendto");
    
//     /* print the server's reply */
//     n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
//     if (n < 0) 
//     error("ERROR in recvfrom");
    
//     char* prefix;
//     int owner;
//     char* allFilesNames;
//     FileEntry fileEntries[40];
//     int nFiles;
    
//     RequestDeformat3(buf, &prefix, &owner, &allFilesNames, fileEntries, &nFiles);
//     printf("%s, %d, %s, %d\n", prefix, owner, allFilesNames, nFiles);
//     for (int i = 0; i < nFiles; i++)
//     {
//       printf("\t%d, %d, %d\n", fileEntries[i].startIndex, fileEntries[i].endIndex, fileEntries[i].isSubdirectory);
//     }

//     free(buf);
//     free(prefix);
//     free(allFilesNames);

//     return 0;
// }

int EstabilishConnection(char* hostname, int portN)
{
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    error("ERROR opening socket");
    return -1;
  } 

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(hostname);
  if (server == NULL) 
  {
    fprintf(stderr,"ERROR, no such host as %s\n", hostname);
    return -2;
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portno);

  return 0;
}

int ReadFile(int owner, char* path, int offset)
{
  char* buf = RequestFormat1("RD-REQ", owner, path, strlen(path), "", offset);

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -1;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -2;
  }
  
  char* prefix;
  int owner;
  char* allFilesNames;
  FileEntry fileEntries[40];
  int nFiles;
  
  RequestDeformat1(buf, &prefix, &owner, &allFilesNames, fileEntries, &nFiles);
}