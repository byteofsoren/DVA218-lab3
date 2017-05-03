
#include "ingsoc.h"

void ingsoc_readMessage(int fileDescriptor, void* data ,struct sockaddr_in *host_info){

    int nOfBytes = sizeof(*host_info);
    char buffer[MAXMSG];

    nOfBytes = recvfrom(fileDescriptor, data, MAXMSG, 0, (struct sockaddr *) host_info, &(nOfBytes));
    if(nOfBytes < 0){
        perror("readMessage - Could not READ data");
        exit(EXIT_FAILURE);
    }
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */

void ingsoc_writeMessage(int fileDescriptor, void* data, struct sockaddr_in *host_info) {

    int nOfBytes;

    nOfBytes = sendto(fileDescriptor, data, 13, 0, (struct sockaddr*)host_info,sizeof(*host_info));
    if(nOfBytes < 0){
        perror("writeMessage - Could not WRITE data\n");
        exit(EXIT_FAILURE);
    }
}
/* XOR Checksum calculator
 * input:
 * data - string to calculate checksum for
 * length - length of string
 * error - If 1, there is a 10% chance for simulated error, else no errors.
 */
 int checkSum(void *data, int length, int error){

    srand(time(NULL));

    unsigned char *ptr;
    int i;
    int check = 4;
    int r;
    int XORvalue;
    int SUMvalue;
    int ERRORvalue = 0;

    ptr = (unsigned char *)data;
    XORvalue = 0;
    SUMvalue = 0;

    for(i = 0; i <= length; i++, ptr++){
        XORvalue ^= *ptr;
        SUMvalue += (*ptr * i);
    }
    /* If chosen, the code will generate a random error with 10% probability */
    if(error == 1){
        r = rand()%10;
        usleep(1000000);
        /* errorValue will be between 1 and the checksum value and will
         * be added to the checksum to simulate an error*/
        if(r == check) {
            ERRORvalue = rand() % (XORvalue ^ SUMvalue) + 1;
        }
    }
    /* Will return the checksum, "errorValue" is zero if no error occurs */
    return((XORvalue ^ SUMvalue) + ERRORvalue);
 }