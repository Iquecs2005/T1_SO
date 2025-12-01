#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

#include "SFSSClient.h"
#include "ServerFormating.h"

static int sockfd = -1;
static int portno, n;
static int serverlen;
static struct sockaddr_in serveraddr;
static struct hostent *server;
static char *hostname;

void error(char *msg) 
{
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) 
{
  if (argc != 3) 
  {
    fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
    exit(0);
  }
  hostname = argv[1];
  portno = atoi(argv[2]);

  EstabilishConnection(hostname, portno);

  IOResponse response;
  ReadFile(2, "/Alo Mundo.txt", 0, &response);

  printf("%s, %d, %s, %d, %s, %d\n", response.prefix, response.owner, response.path, response.pathlen, response.payload, response.offset);

  free(response.prefix);
  free(response.path);
  free(response.payload);

  WriteFile(2, "/Alo Mundo.txt", "aaaaaaaaaaaaaaaa", 16, &response);

  printf("%s, %d, %s, %d, %s, %d\n", response.prefix, response.owner, response.path, response.pathlen, response.payload, response.offset);

  free(response.prefix);
  free(response.path);
  free(response.payload);

  DirResponse dirResponse;
  CreateDir(2, "/", "c", &dirResponse);

  printf("%s, %d, %s, %d\n", dirResponse.prefix, dirResponse.owner, dirResponse.path, dirResponse.pathlen);

  free(dirResponse.prefix);
  free(dirResponse.path);

  RemoveDir(2, "/", "c", &dirResponse);

  printf("%s, %d, %s, %d\n", dirResponse.prefix, dirResponse.owner, dirResponse.path, dirResponse.pathlen);

  free(dirResponse.prefix);
  free(dirResponse.path);

  ListDirResponse listDirResp;
  ListDir(2, "/", &listDirResp);
  printf("%s %d %s %d\n", listDirResp.prefix, listDirResp.owner, listDirResp.allFilesNames, listDirResp.nrNames);
  for (int i = 0; i < listDirResp.nrNames; i++)
  {
    printf("%d, %d, %d\n", listDirResp.fstlstpositions[i].startIndex, listDirResp.fstlstpositions[i].endIndex, listDirResp.fstlstpositions[i].isSubdirectory);
  }

  free(listDirResp.prefix);
  free(listDirResp.allFilesNames);

  return 0;
}

int EstabilishConnection(char* localHostname, int portN)
{
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
  {
    error("ERROR opening socket");
    return -1;
  } 

  /* gethostbyname: get the server's DNS entry */
  server = gethostbyname(localHostname);
  if (server == NULL) 
  {
    fprintf(stderr,"ERROR, no such host as %s\n", localHostname);
    return -2;
  }

  /* build the server's Internet address */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr, 
  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
  serveraddr.sin_port = htons(portN);

  hostname = (char*)malloc(sizeof(char) * strlen(localHostname));
  hostname[0] = '\0';
  strcpy(hostname, localHostname);
  portno = portN;

  return 0;
}

int ReadFile(int owner, char* path, int offset, IOResponse* response)
{
  if (sockfd == -1)
  {
    printf("No established connection\n");
    return -1;
  }

  char* buf = RequestFormat1("RD-REQ", owner, path, strlen(path), "", offset);

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -2;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -3;
  }
  
  RequestDeformat1(buf, &(response->prefix), &(response->owner), &(response->path), &(response->pathlen), &(response->payload), &(response->offset));

  free(buf);

  return 0;
}

int WriteFile(int owner, char* path, char* content, int offset, IOResponse* response)
{
  if (sockfd == -1)
  {
    printf("No established connection\n");
    return -1;
  }

  char* buf = RequestFormat1("WR-REQ", owner, path, strlen(path), content, offset);

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -2;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -3;
  }
  
  RequestDeformat1(buf, &(response->prefix), &(response->owner), &(response->path), &(response->pathlen), &(response->payload), &(response->offset));

  free(buf);

  return 0;
}

int CreateDir(int owner, char* path, char* dirname, DirResponse* response)
{
  if (sockfd == -1)
  {
    printf("No established connection\n");
    return -1;
  }

  char* buf = RequestFormat1("DC-REQ", owner, path, strlen(path), dirname, strlen(dirname));

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -2;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -3;
  }
  
  RequestDeformat2(buf, &(response->prefix), &(response->owner), &(response->path), &(response->pathlen));

  free(buf);

  return 0;
}

int RemoveDir(int owner, char* path, char* dirname, DirResponse* response)
{
  if (sockfd == -1)
  {
    printf("No established connection\n");
    return -1;
  }

  char* buf = RequestFormat1("DR-REQ", owner, path, strlen(path), dirname, strlen(dirname));

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -2;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -3;
  }
  
  RequestDeformat2(buf, &(response->prefix), &(response->owner), &(response->path), &(response->pathlen));

  free(buf);

  return 0;
}

int ListDir(int owner, char* path, ListDirResponse* response)
{
  if (sockfd == -1)
  {
    printf("No established connection\n");
    return -1;
  }

  char* buf = RequestFormat2("DL-REQ", owner, path, strlen(path));

  serverlen = sizeof(serveraddr);
  n = sendto(sockfd, buf, BufferSize(), 0, &serveraddr, serverlen);
  if (n < 0) 
  {
    error("ERROR in sendto");
    return -2;
  }
  
  n = recvfrom(sockfd, buf, BufferSize(), 0, &serveraddr, &serverlen);
  if (n < 0) 
  {
    error("ERROR in recvfrom");
    return -3;
  }
  
  RequestDeformat3(buf, &(response->prefix), &(response->owner), &(response->allFilesNames), &(response->fstlstpositions), &(response->nrNames));

  free(buf);

  return 0;
}