
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
    memset(insoci->data, 0, 255);
}

void ingsoc_pretty_print(ingsoc *s){
    char t[] = "\e[032mtrue\e[0m";
    char f[] = "\e[031mfalse\e[0m";
    printf("ACK=%s, FIN=%s, RES=%s, SYN=%s\n",
            s->ACK?t:f,
            s->FIN?t:f,
            s->RES?t:f,
            s->SYN?t:f);
    printf(" ACKnr=\e[036m%ld\e[0m\n SEQ=\e[036m%ld\e[0m\n clientID=\e[036m%ld\e[0m\n", s->ACKnr, s->SEQ, s->clientID);
    printf("cksum=\e[033m%d\e[0m, length=\e[033m%d\e[0m\n", s->cksum, s->length);
    printf("Data='%s'\n\n",s->data);
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

size_t convert_size_t(char *buffer, size_t pos, size_t data){
    int length = sizeof(data);
    char temp[length];
    memset(temp, ' ' , length);
    //printf("temp=%s, pos=%ld\n", temp, pos);
    snprintf(temp, length, "%ld", data);
    memcpy(buffer + pos, temp, sizeof(size_t));
    return length + pos;
}

size_t convert_short(char *buffer, size_t pos, short data){
    int length = sizeof(data);
    char temp[length];
    memset(temp, ' ' , length);
    //printf("temp=%s, pos=%ld\n", temp, pos);
    snprintf(temp, length, "%d", data);
    memcpy(buffer + pos, temp, sizeof(size_t));
    return length + pos;
}

int toSerial(ingsoc *package, char *out){
    int bytes = 0;
    bytes = (int) sizeof(ingsoc) + 1;
    char buffer[bytes + 3];
    memset(buffer, 'E' , bytes + 2);
    buffer[bytes+3] = '\0';
    //buffer[0] = package->ACK;
    buffer[0] = (package->ACK) ? 't' : 'f';
    //printf("ACK in buffer is %c\n", buffer[0]);
    buffer[1] = (package->FIN) ? 't' : 'f';
    buffer[2] = (package->RES) ? 't' : 'f';
    buffer[3] = (package->SYN) ? 't' : 'f';
    int counter = 4;
    //printf("buffer %s\n" ,buffer);
    /*char temp[sizeof(size_t)];
    snprintf(temp, sizeof(size_t), "%ld", package->ACKnr);
    memcpy(buffer + counter, temp, sizeof(size_t));
*/
    counter = convert_size_t(buffer, counter, package->ACKnr);
    counter = convert_size_t(buffer, counter, package->SEQ);
    counter = convert_size_t(buffer, counter, package->clientID);
    counter = convert_short(buffer,  counter, package->cksum);
    counter = convert_short(buffer, counter, package->length);
    memcpy(buffer + counter, package->data, 255);

    memcpy(out, buffer, bytes);
    return bytes;
}
size_t revert_size_t(char *buffer, size_t pos, size_t *data){
    size_t length = sizeof(data);
    char temp[sizeof(size_t)];
    memcpy(temp, buffer + pos, sizeof(size_t));
    //printf("temp=%s",temp);
    *data = atoi(temp);
    return length + pos;
}

size_t revert_short(char *buffer, size_t pos, short *data){
    size_t length = sizeof(data);
    char temp[sizeof(size_t)];
    memcpy(temp, buffer + pos, sizeof(size_t));
    //printf("temp=%s",temp);
    *data = atoi(temp);
    return length + pos;
}


ingsoc *fromSerial(char *buffer){
    ingsoc *pack= (ingsoc*)calloc(1, sizeof(ingsoc));
    //size_t bytes = sizeof(ingsoc);
    pack->ACK = (buffer[0] == 't') ? true:false;
    pack->FIN = (buffer[1] == 't') ? true:false;
    pack->RES = (buffer[2] == 't') ? true:false;
    pack->SYN = (buffer[3] == 't') ? true:false;
    /*char temp[sizeof(size_t)];
    memcpy(temp, buffer + 4, sizeof(size_t));
    printf("temp=%s",temp);
    pack->ACKnr = atoi(temp);
    */
    size_t counter = 4;
    counter = revert_size_t(buffer, counter, &pack->ACKnr);
    counter = revert_size_t(buffer, counter, &pack->SEQ);
    counter = revert_size_t(buffer, counter, &pack->clientID);
    counter= revert_short(buffer, counter, &pack->cksum);
    counter= revert_short(buffer, counter, &pack->length);
    memcpy(pack->data, buffer + counter, 255);
    return pack;
}

short ingsoc_cksum(char *buffer, int length){
    short csum = 0;
    for (int i = 0; i < length; ++i) {
        csum += buffer[i];
    }
    return csum;
}

void ingsoc_readMessage(int fileDescriptor, ingsoc* pack,struct sockaddr_in *host_info){

    unsigned nOfBytes = sizeof(*host_info);
    int dataRead = 0;
    char *buffer = (char *) calloc(sizeof(ingsoc), sizeof(char));

    dataRead = recvfrom(fileDescriptor, buffer, MAXMSG, 0, (struct sockaddr *) host_info, &(nOfBytes));
    if(dataRead < 0){
        perror("readMessage - Could not READ data");
        exit(EXIT_FAILURE);
    }
    ingsoc *tpack = fromSerial( buffer);
    printf("prit sock in ingsoc_readMessage\n");
    ingsoc_pretty_print(tpack);
    memcpy(pack, tpack, sizeof(ingsoc));

}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */

void ingsoc_writeMessage(int fileDescriptor, ingsoc* data, int length, struct sockaddr_in *host_info) {

    int nOfBytes = length;
    char *buffer = (char*)calloc(sizeof(ingsoc), sizeof(char));
    //short cksum = data->cksum;
    data->cksum = 0;
    toSerial(data,buffer);

    data->cksum = ingsoc_cksum(buffer, sizeof(buffer));
    printf("Sending this struct\n");
    ingsoc_pretty_print(data);
    nOfBytes = sendto(fileDescriptor, buffer, sizeof(buffer), 0, (struct sockaddr*)host_info,sizeof(*host_info));
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
