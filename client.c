
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin_port = htons(port);
  /* Get info about host. */
  hostInfo = gethostbyname(hostName);
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */
void writeMessage(int fileDescriptor, char *message) {
  int nOfBytes;

  nOfBytes = write(fileDescriptor, message, strlen(message) + 1);
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}

int readMessageFromServer(int fileDescriptor){
    char buffer[MAXMSG];
    int nOfBytes;

    nOfBytes = read(fileDescriptor, buffer, MAXMSG);
    if (nOfBytes < 0) {
        perror("Could not read data from client\n");
        exit(EXIT_FAILURE);
    }else{
        if(nOfBytes == 0)
            return(-1);
        else
            printf("\n>Incomming message: %s\n", buffer);
        return(0);
    }
}
void *reader(void *fileDescriptor_in){
    // This function is called by pthread create
    // And is used to display the data from the server
    int fileDescriptor = 0;
    // int a = *((int*) i);
    fileDescriptor = *((int*) fileDescriptor_in);
    while (1){
        usleep(5000);
        if (readMessageFromServer(fileDescriptor) == -1) {
            //printf("Empty data from server\n");
        }
    }
    return 0;
}


