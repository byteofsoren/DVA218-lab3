#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"
#include "client.h"
/* XOR Checksum calculator
 * input:
 * data - string to calculate checksum for
 * length - length of string
 * error - If 1, there is a 10% chance for error, else no errors.
 */

 int checkSum(void *data, int length, int error){

    srand(time(NULL));

    unsigned char *ptr;
    int i;
    int check = 4;
    int r;
    int XORvalue;
    int SUMvalue;
    int errorValue = 0;

    ptr = (unsigned char *)data;
    XORvalue = 0;
    SUMvalue = 0;

    for(i = 0; i <= length; i++, ptr++){
        XORvalue ^= *ptr;
        SUMvalue += (*ptr * i);
    }
    if(error == 1){
        r = rand()%10;
        usleep(10000);

        if(r == check) {

            errorValue = rand() % (XORvalue ^ SUMvalue);
        }
    }

    return((XORvalue ^ SUMvalue) + errorValue);
 }
/* errorGen - Generates random errors with n% probability
 * Input: error - size of error
 * Output: A random int or 0.
 */
int errorGen(int error){

    int check = 4;
    int r = rand()%10;



    if(r == check) {
        usleep(10000);
        error = rand() % error;
        return (error);
    }
    else return(0);
}