#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <net/if.h>

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

char HW_CONECT[200] = "wlp1s0";

void _initSocketAddress(struct sockaddr_in6 *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin6_family = AF_INET6;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin6_port = htons(port);
  /* Get info about host. */
  //name->sin6_scope_id=if_nametoindex(HW_CONECT);
  name->sin6_scope_id=3;  // Wierles interface is 3 on most cases.
  name->sin6_flowinfo=0;
  //hostInfo = gethostbyname(hostName);  //Obsolite
  unsigned char buf[sizeof(struct in6_addr)];
  int t = inet_pton(AF_INET6, hostName, buf);
  if (t <= 0){
    printf("inet_pton genereated a1n error");
    exit(EXIT_FAILURE);
  }
  hostInfo = gethostbyaddr(buf, sizeof(buf), AF_INET6);
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin6_addr = *(struct in6_addr *)hostInfo->h_addr;
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */
void _writeMessage(int fileDescriptor, char *message) {
  int nOfBytes;

  nOfBytes = write(fileDescriptor, message, strlen(message) + 1);
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}



void client_main(char *addres)
{
    int sock = 0;
    sock = socket(PF_INET6, SOCK_DGRAM, 0);
    if(sock < 0){
        perror("Could not create a socet\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in6 serverName;
    _initSocketAddress(&serverName, addres, PORT);
    _writeMessage(sock, "Hello Hampus");
    
}
