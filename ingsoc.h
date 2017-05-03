#ifndef INGSOC
#define INGSOC
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <unistd.h>
#include <stdbool.h>

/*
    ___      ___
    \  \    /  /
     \  \  /  /
     [ ingsoc ]
       \    /
        \__/

 */
typedef struct{

    bool ACK, FIN, RES;
    size_t ACKnr, SEQ, clientID;
    short cksum, length;
    void *data;

}ingsoc;

int checkSum(void *data, int length, int error);
#endif /* ifndef INGSOC */
