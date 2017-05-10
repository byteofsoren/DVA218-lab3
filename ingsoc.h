#ifndef INGSOC
#define INGSOC

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <net/if.h>
#include "server.h"
#include <time.h>

#define PORT 5555
#define MAXMSG 512
/*
    ___      ___
    \  \    /  /
     \  \  /  /
     [ ingsoc ]
       \    /
        \__/

 */
typedef struct{

    bool ACK, FIN, RES, SYN;
    size_t ACKnr, SEQ, clientID;
    short cksum, length;
    char data[255];

}ingsoc;

void ingsoc_init(ingsoc *ingsoc_i);
void ingsoc_seqnr(ingsoc *in);
int toSerial(ingsoc *pakcage, char *out);
ingsoc *fromSerial(char *buffer);
void ingsoc_readMessage(int fileDescriptor, ingsoc* pack,struct sockaddr_in *host_info);
void ingsoc_writeMessage(int fileDescriptor, ingsoc* data, int length, struct sockaddr_in *host_info);
int checkSum(void *data, int length, int error);
#endif /* ifndef INGSOC */
