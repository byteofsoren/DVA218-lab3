#ifndef INGSOC
#define INGSOC

#include <pthread.h>
#include <time.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <net/if.h>
#include "server.h"
/*
    ___      ___
    \  \    /  /
     \  \  /  /
     [ ingsoc ]
       \    /
        \__/

 * */


int checkSum(void *data, int length, int error);
int errorGen(int error);
#endif /* ifndef INGSOC */
