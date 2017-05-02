#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

/* XOR Checksum calculator
 * input:
 * data - string to calculate checksum for
 * length - length of string
 */

 int checkSum(void *data, int length){

    unsigned char *ptr;
    int i;
    int XORvalue;
    int SUMvalue;

    ptr = (unsigned char *)data;
    XORvalue = 0;
    SUMvalue = 0;

    for(i = 0; i <= length; i++, ptr++){
        XORvalue ^= *ptr;
        SUMvalue += (*ptr * i);
    }
    return(XORvalue^SUMvalue);
 }