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
#define MAXMSG 4096
#define MAX_DATA 32
#define READ_FILE
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
    unsigned short cksum, length;
    char data[MAX_DATA];

}ingsoc;

void ingsoc_show_error_chance();
void ingsoc_init(ingsoc *ingsoc_i);
void ingsoc_seqnr(ingsoc *in);
int ingsoc_readMessage(int fileDescriptor, ingsoc* data ,struct sockaddr_in *host_info);
void ingsoc_writeMessage(int fileDescriptor, ingsoc* data, int length, struct sockaddr_in *host_info);
int checkSum(void *data, int length, int error);
void input(char* msg);
u_int CheckSumConf(void *cnf);
size_t ingsoc_randomNr(size_t min, size_t max);
#endif /* ifndef INGSOC */
