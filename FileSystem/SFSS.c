#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "FileOperations.h"
#include "ServerFormating.h"

#define MAXPATHSIZE 64

void error(char *msg);
char* TreatRequest(char* buf);

int main(int argc, char **argv) 
{
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  
  char buf[BufferSize()]; /* message buf */

  /*
   * check command line arguments
   */
  if (argc != 2) 
  {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the parent socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
	     (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) 
  {
    /*
     * recvfrom: receive a UDP datagram from a client
     */
    bzero(buf, BufferSize());
    n = recvfrom(sockfd, buf, BufferSize(), 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
  
    char* reply = TreatRequest(buf);

    /*
     * gethostbyaddr: determine who sent the datagram
     */
    hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
			  sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n",
	   hostp->h_name, hostaddrp);

    /*
     * sendto: echo the input back to the client
     */
    n = sendto(sockfd, reply, BufferSize(), 0,
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
      error("ERROR in sendto");

    free(reply);
  }
}

void error(char *msg) 
{
  perror(msg);
  exit(1);
}

char* TreatRequest(char* buf)
{
  if (strcmp(buf, "RD-REQ") == 0)
  {
    printf("Read Request\n");
    
    char* prefix;
    int owner;
    char* path;
    int pathlen;
    char* payload;
    int offset;

    RequestDeformat1(buf, &prefix, &owner, &path, &pathlen, &payload, &offset);
    printf("%s %d %s %d %s %d\n", prefix, owner, path, pathlen, payload, offset);

    unsigned char* buffer = (unsigned char*)malloc(sizeof(char) * 16);
    int status = ReadOperation(owner, path, pathlen, buffer, offset);

    if (status != 0)
      offset = status;

    char* reply = RequestFormat1("RD-REP", owner, path, pathlen, buffer, offset);

    free(prefix);
    free(path);
    free(payload);

    return reply;
  }
  else if (strcmp(buf, "WR-REQ") == 0)
  {
    printf("Write Request\n");

    char* prefix;
    int owner;
    char* path;
    int pathlen;
    char* payload;
    int offset;

    RequestDeformat1(buf, &prefix, &owner, &path, &pathlen, &payload, &offset);
    printf("%s %d %s %d %s %d\n", prefix, owner, path, pathlen, payload, offset);

    int status = WriteOperation(owner, path, pathlen, payload, offset);

    if (status != 0)
      offset = status;

    char* reply = RequestFormat1("WR-REP", owner, path, pathlen, "", offset);

    return reply;
  }
  else if (strcmp(buf, "DC-REQ") == 0)
  {
    printf("Create Directory Request\n");

    char* prefix;
    int owner;
    char* path;
    int pathlen;
    char* dirname;
    int dirlen;

    RequestDeformat1(buf, &prefix, &owner, &path, &pathlen, &dirname, &dirlen);
    printf("%s %d %s %d %s %d\n", prefix, owner, path, pathlen, dirname, dirlen);

    int status = DirCreateOperation(owner, path, pathlen, dirname, dirlen);

    if (status != 0)
    {
      dirlen = status;
    }
    else
    {
      char* newPath = (char*)malloc(sizeof(char) * (strlen(path) + strlen(dirname)));
      strcpy(newPath, path);
      strcat(newPath, dirname);
      free(path);
      path = newPath;
      pathlen = strlen(path);
    }

    char* reply = RequestFormat2("DC-REP", owner, path, pathlen);

    free(prefix);
    free(path);
    free(dirname);

    return reply;
  }
  else if (strcmp(buf, "DR-REQ") == 0)
  {
    printf("Directory Remove Request\n");

    char* prefix;
    int owner;
    char* path;
    int pathlen;
    char* dirname;
    int dirlen;

    RequestDeformat1(buf, &prefix, &owner, &path, &pathlen, &dirname, &dirlen);
    printf("%s %d %s %d %s %d\n", prefix, owner, path, pathlen, dirname, dirlen);

    int status = DirRemoveOperation(owner, path, pathlen, dirname, dirlen);

    if (status != 0)
    {
      pathlen = status;
    }

    char* reply = RequestFormat2("DR-REP", owner, path, pathlen);

    free(prefix);
    free(path);
    free(dirname);

    return reply;
  }
  else if (strcmp(buf, "DL-REQ") == 0)
  {
    printf("Directory List Request\n");

    char* prefix;
    int owner;
    char* path;
    int pathlen;

    RequestDeformat2(buf, &prefix, &owner, &path, &pathlen);
    printf("%s %d %s %d\n", prefix, owner, path, pathlen);

    FileEntry* filesEntrys;
    int nFiles;
    char* allfilesnames = DirListOperation(owner, path, pathlen, &filesEntrys, &nFiles);

    printf("NFiles: %d\n", nFiles);
    char* message = RequestFormat3("DL-REP", owner, allfilesnames, filesEntrys, nFiles);
    
    free(prefix);
    free(allfilesnames);
    free(filesEntrys);

    return message;
  }

  char* reply = (char*)malloc(BufferSize());
  reply[0] = '\0';
  return reply;
}