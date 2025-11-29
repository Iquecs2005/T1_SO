#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
#define MAXPATHSIZE 64

void error(char *msg);
int parse (char *buf, int *cmd, char *name);
void TreatRequest(char* buf, int size);
char** GetParameters(char* buf, int* size);

int main(int argc, char **argv) 
{
  int sockfd; /* socket */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */

  char name[BUFSIZE];   // name of the file received from client
  int cmd;              // cmd received from client

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
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,
		 (struct sockaddr *) &clientaddr, &clientlen);
    if (n < 0)
      error("ERROR in recvfrom");
  
    TreatRequest(buf, n);

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
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

    /*
     * sendto: echo the input back to the client
     */
    n = sendto(sockfd, buf, strlen(buf), 0,
	       (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
      error("ERROR in sendto");
  }
}

void error(char *msg) 
{
  perror(msg);
  exit(1);
}

int parse (char *buf, int *cmd, char *name) {
    char *cmdstr;

    cmdstr = strtok(buf,";");
        name = strtok(NULL,"\0");
    cmd = atoi(cmdstr);
}

void TreatRequest(char* buf, int size)
{
  if (size < 2)
      return;
  
  char operation = buf[0];
  int owner = (int)(buf[1] - '0');

  if (owner > 5 || owner < 1)
      return;

  int size;
  char** parameters = GetParameters(buf, &size);

  for (int i = 0; i < size; i++)
  {
    printf("%s\n", parameters[i]);
  }

  switch (operation)
  {
  case 'r':
    printf("Read Request\n");
    printf("Buf: %s\n", buf);
    
    break;
  case 'R':
    printf("Read Data Request\n");
    printf("%d\n", owner);
    break;
  case 'w':
    printf("Write Request\n");
    printf("%d\n", owner);
    break;
  case 'W':
    printf("Write Data Request\n");
    printf("%d\n", owner);
    break;
  default:
      break;
  }
}

char** GetParameters(char* buf, int* size)
{
  int nTokens = 0;
  char* copy;

  copy = (char*)malloc(sizeof(char) * strlen(buf));
  strcpy(copy, buf);
  
  char* token = strtok(copy, ";");

  while (token != NULL) 
  {
    nTokens++;
    token = strtok(NULL, ";");
  }

  free(copy);
  copy = (char*)malloc(sizeof(char) * strlen(buf));
  strcpy(copy, buf);

  char** tokensList = (char**)malloc(nTokens * sizeof(char*));

  tokensList[0] = strtok(buf, ";");
  for (int i = 1; i < nTokens; i++) 
  {
    tokensList[i] = strtok(NULL, ";");
  }

  *size = nTokens;

  return tokensList;
}