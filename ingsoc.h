#ifndef INGSOC
#define INGSOC
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <unistd.h>

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
