
#include "ingsoc.h"


void ingsoc_init(ingsoc *insoci)
{
    insoci->SYN=false;
    insoci->clientID = getpid();
    insoci->FIN=false;
    insoci->RES=false;
    insoci->ACK=false;
    insoci->ACKnr=0;
    insoci->SEQ=0;
    insoci->cksum=0;
    insoci->length=0;
    insoci->data=0;
}


size_t ingsoc_randomNr(size_t min, size_t max){
// This is my random number generator
  size_t result = 0 , low = 0 ,hig = 0;

  if ( min < max) {
    low = min;
    hig = max + 1;        // include the max result in the output
  }  else {
    low = max + 1;        // include the max result in the output
    hig = min;
  }
  result = rand() % (hig - low)+ low;  /* What this do is better expalined with an exampel.
  Lest say you get;
    rand()=234532   hig=20    low=5
    first evaluet (hig-low)=15
    then 234532 % 15 = 7
    last 7 + 5 = 12  */
  return result;
}


void ingsoc_seqnr(ingsoc *in)
{
    static size_t startNr=0;
    if(startNr == 0){
        startNr = ingsoc_randomNr(10,2000000000);
    }else{
        startNr++;
    }
    in->SEQ = startNr;


}

void ingsoc_readMessage(int fileDescriptor, void* data ,struct sockaddr_in *host_info){

    unsigned nOfBytes = sizeof(*host_info);
    int dataRead = 0;
//    char buffer[MAXMSG];

    dataRead = recvfrom(fileDescriptor, data, MAXMSG, 0, (struct sockaddr *) host_info, &(nOfBytes));
    if(dataRead < 0){
        perror("readMessage - Could not READ data");
        exit(EXIT_FAILURE);
    }
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */

void ingsoc_writeMessage(int fileDescriptor, void* data, int length, struct sockaddr_in *host_info) {

    int nOfBytes;

    nOfBytes = sendto(fileDescriptor, data, length, 0, (struct sockaddr*)host_info,sizeof(*host_info));
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
