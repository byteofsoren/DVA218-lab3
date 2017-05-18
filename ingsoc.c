
#include "ingsoc.h"
#include "newspeak.h"

#define CHANCE  (short) ingsoc_randomNr(0,100)
#define MAX_JAIL 10
#define CHANCE_TO_GET_CHKSUM_ERROR 10
#define CHANCE_TO_GET_OUT_ORDER 10
//#define ERROR_MESSAGE_ON_NO_SOCKET
//#define ERROR_MESSAGE_IN_GENERATOR
ingsoc jail[MAX_JAIL];
short jailer[MAX_JAIL];
short number_of_inmates = 0;

void ingsoc_init(ingsoc *insoci)
{
    /* This function is used to initialize the ingsock structure
     * to default values.*/
    insoci->SYN=false;
    insoci->clientID = getpid();
    insoci->FIN=false;
    insoci->RES=false;
    insoci->ACK=false;
    insoci->ACKnr=0;
    insoci->SEQ=0;
    insoci->cksum=0;
    insoci->length=0;
    insoci->data[0] = '\0';
}

size_t convert_size_t(char *buffer, size_t pos, size_t data){
    /* This function copies the data type size_t to the array
     * it works by byte by byte copy from en memory region to the next
     * memory region in this case we don't get readable text in the buffer
     * because sice_t is 8 byte big and if we copy one of those bytes to a
     * char we get for human unreadable data */
    int length = sizeof(size_t);
    char temp[length];
    /* A buffer is created to temporary store the data
     * Then we set the entire buffer to 0 for safety */
    memset(temp, 0 , length);
    /* Bitwise copy of data form the data type size_t to the char array */
    memcpy(temp,&data,length);
    /* Copy the data form the temporary buffer to the outside buffer*/
    memcpy(buffer + pos, temp, sizeof(size_t));
    /* We now have to update our position in the buffer
     * there of length + pos */
    return length + pos;
}

size_t convert_short(char *buffer, size_t pos, unsigned short data){
    /* Doe's the same ass the convert_sicze_t but for the
     * data type unsigned short */
    int length = sizeof(short);
    char temp[length];
    memset(temp, 0 , length);
    memcpy(temp,&data,length);
    memcpy(buffer + pos, temp, sizeof(short));
    return length + pos;
}

int toSerial(ingsoc *package, char *out){
    /* This function serialize the struct int tho a char array
     * It does that by byce coppy*/
    int bytes = 0;
    bytes = (int) sizeof(ingsoc) + 10;
    /* First we created a int to store the pos in the array
     * Then we create a buffer on the stack */
    char buffer[bytes + 3];
    memset(buffer, 0 , bytes + 2);
    buffer[bytes+3] = '\0';
    /* For safety we write zeros tho the entire array
     * What happens below is we use the first 4 bytes to store the boolean value*/
    buffer[0] = (package->ACK) ? 't' : 'f';
    buffer[1] = (package->FIN) ? 't' : 'f';
    buffer[2] = (package->RES) ? 't' : 'f';
    buffer[3] = (package->SYN) ? 't' : 'f';
    /* The state abode is an compressed if else statement, for example
     * buffer[0] = (package->ACX ? 't' : ':f' Can be written as
     * if(package-->ACK) buffer[0]='t';
     * else buffer[0]='f'
     * Its just more compact*/
    int counter = 4;
    /* The counter from this point just keeps track of the pos in the array
     * then the different functions copies the data form the struct to the
     * buffer*/
    counter = convert_size_t(buffer, counter, package->ACKnr);
    counter = convert_size_t(buffer, counter, package->SEQ);
    counter = convert_size_t(buffer, counter, package->clientID);
    counter = convert_short(buffer,  counter, package->cksum);
    counter = convert_short(buffer, counter, package->length);
    /* Bitwise copy of the data in the struct to the buffer array*/
    memcpy(buffer + counter, package->data, MAX_DATA);
    /* Then we copy the buffer out of stack back to the out inter */
    memcpy(out, buffer, bytes);
    return bytes;
}
size_t revert_size_t(char *buffer, size_t pos, size_t *data){
    /* This function reads the data out of the buffer and assigns it back to
     * the pointer *data */
    size_t length = sizeof(size_t);
    char temp[length];
    /* Create a temporary buffer to store a part of the buffer
     * Then we bitwise copy the data back out of the temporary buffer */
    memcpy(temp, buffer + pos, sizeof(size_t));
    memcpy(data,temp,length);
    /* We still need to keep our place in the buffer so return the new position
     * to the counter in fromSerial */
    return length + pos;
}

size_t revert_short(char *buffer, size_t pos, unsigned short *data){
    /* Same as revert_size_t but for the data type unsigned short */
    size_t length = sizeof(short);
    char temp[length];
    memcpy(temp, buffer + pos, sizeof(short));
    memcpy(data,temp,length);
    return length + pos;
}

ingsoc *fromSerial(char *buffer){
    /* Does the exact opposite of toSerial it converts the buffer to an ingsock*/
    ingsoc *pack= (ingsoc*)calloc(1, sizeof(ingsoc));
    /* Creating the strut on the heap.
     * The first 4 bytes of the Buffer contains the information nedded to set
     * the structs boolean variable */
    pack->ACK = (buffer[0] == 't') ? true:false;
    pack->FIN = (buffer[1] == 't') ? true:false;
    pack->RES = (buffer[2] == 't') ? true:false;
    pack->SYN = (buffer[3] == 't') ? true:false;
    /* To understand how that syntax works please read
     * the toSerial function abode */
    size_t counter = 4;
    /* The counter above keeps the place in the data. The functions below reads
     * the data form the buffer and assign them tho the right field in the
     * structure, notice that the order in this function is the same as
     * toSerial function */
    counter = revert_size_t(buffer, counter, &pack->ACKnr);
    counter = revert_size_t(buffer, counter, &pack->SEQ);
    counter = revert_size_t(buffer, counter, &pack->clientID);
    counter = revert_short(buffer, counter, &pack->cksum);
    counter = revert_short(buffer, counter, &pack->length);
    memcpy(pack->data, buffer + counter, sizeof(ingsoc));
    return pack;
}

short ingsoc_cksum(char *buffer, int length){
    /* Calculates the check sum by adding up the values that is defined by char */
    short tsum = 0;
    /* First we define a tsum variable that is used as a total summary
     * In the for loop below we iterate each char in the buffer and
     * take the value in that char + the tsum of previously summary */
    for (int i = 0; i < length; ++i) {
        tsum += buffer[i];
    }
    /* We return the total value of the buffer */
    return tsum;
}
u_int CheckSumConf(void *cnf)
{
    size_t i;
    u_int chk=0;
    unsigned char *data;

    data = cnf;
    for (i=2; i < sizeof(ingsoc); i++) chk += *data++;
    return chk;
}

size_t ingsoc_randomNr(size_t min, size_t max){
    /* Random number generator */
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
    /* Creating a sequence number for the sliding window is done by
     * at first randomly beginning on a random number, then after
     * we randomly created a number we increase the number by 1 for every
     * package that is going to be sent*/
    static size_t startNr=0;
    /* This is done by using a static variable that is initialized to 0
     * the fist time the function is started.*/
    if(startNr == 0){
        startNr = ingsoc_randomNr(10,2000000000);
    }else{
        /* After that we increase the value by 1 each time the function runs */
        startNr++;
    }
    /* We sett the number to the structure before exiting the function */
    in->SEQ = startNr;


}
void input(char* msg)		//my input function from user
{
    char dummy;
    printf("\e[032m");
    fgets(msg, MAXMSG, stdin);
    printf("\e[0m");
    if (*(msg + (strlen(msg) - 1)) == '\n')		//if the last char is \n. Changes it to \0
    {
        *(msg + (strlen(msg) - 1)) = '\0';
    }
    else		//if the last char is not \n
    {
        do
        {   // loop until the new-line is read to remove keyboard buffer
            dummy = getchar();
        } while (dummy != '\n');

    }
}

int ingsoc_readMessage(int fileDescriptor, ingsoc* data ,struct sockaddr_in *host_info){
    /* ingsoc_readMesssage is used to read data form sockets back to who ever
     * function that called it form the beginning. The data pointer in
     * the argument section is used as an output, The return value form this
     * function is zero for success or -1 for no package received. */

    unsigned nOfBytes = sizeof(*host_info);
    int dataRead = 0, sentChSum = 0;
    /* First we try to read data form socket and store the data in the data
     * pointer in the argument */

    dataRead = recvfrom(fileDescriptor, data, MAXMSG, 0, (struct sockaddr *) host_info, &(nOfBytes));
    if(dataRead < 0){
        /* If the was not received we get an error message and returns with -1
         * to tell the calling function that data wasn't read */
        perror("readMessage - Could not READ data");
        return -1;
    }
    /* Checksum calculation. The check sum is calculated by first convert the
     * entire struct to an char array and then preform a summarisation of the
     * chars. For long messages that should be pretty secure way to solve the
     * checksum problem */
    sentChSum = data->cksum;
    data->cksum = 0;
    /* First we stored the checksum from the received data struct and then we
     * set it to zero because we want to recalculate the checksum.
     * Now we are going to initialize a buffer that is going to
     * be used in the serialisation */
    size_t buffer_size = sizeof(ingsoc) + 10;
    char buff[buffer_size];
    char *buffer = buff;
    memset(buffer, 0, buffer_size);
    /* send the data pointer and buffer to serialization */
    toSerial(data,buffer);
    /* recalculate the checksum based on the buffer */
    data->cksum = ingsoc_cksum(buffer, buffer_size);
    if(data->cksum == sentChSum)
    {
        return 0;
    }
    else
    {
        return -1;
    }
    return dataRead;
}

short _numberInJail(){
    /* Returns the number of packages in jail
     * by utilizing a loop */
    short total = 0;
    while((jailer[total] == 1) & (total <= MAX_JAIL)){ total++; } // Loops the jail to count jailed item
    return total;
}


bool _sendToJail(ingsoc *in){
    /* Sends a package to jail
     * Id does that by checking for empty space in the jailer global
     * returns true if the package was successfully stored in jail ore
     * false if unsuccessful */
    int i = 0;
    printf("SEQ \e[38;5;205m%ld\e[0m to jail\n", in->SEQ);
    while((jailer[i] == 1) & (i <= MAX_JAIL)){ i++; } // Finds a empty spot in jail
    if(i <= MAX_JAIL){
        jail[i] = *in;
        jailer[i] = 1;
        number_of_inmates++;
        return true;
    }
    return false; //NO jail space left
}
ingsoc *_getFromJail_byID(int id){
    /* Get form jail by id is a function mostly used by get first from jail
     * function. The function works by returning the strut stored in the jail
     * array. Also sets the jail place to 0 */
    printf("SEQ \e[38;5;40m%ld\e[0m out of jail\n", jail[id].SEQ);
    jailer[id] = 0;
    number_of_inmates--;
    return &jail[id];
}
ingsoc *_returnFromJail(){
    /* Returns a random package form jail */
    int i =0;
    int loop_counter = 0;
    do{
        i = ingsoc_randomNr(0,MAX_JAIL);
        //printf("getformjail loop_counter=%d i=%d number_of_inmates=%d\n", loop_counter, i, number_of_inmates);
        loop_counter++;
        if(loop_counter > 60) {
            printf("\e[38;5;13mUgly solution to the infinity loop problem\n\e[0m\n");
            break;
        }
    } while(jailer[i] == 0);
    return _getFromJail_byID(i);
}

short errorGenerator( ingsoc* data ){
    /* error generator generates error in the communication
     * The generator uses chance to generate a certain type of error.
     * The chance to get certain type of error is stored in a couple of defines
     * in the top of the file. The return value form this function works on a
     * binary level that allow return of multiple states*/

    //printf("errorGenerator \e[032mStart\e[0m\n");
    /* The generator have 2 different states that it need to remember between
     * executions */
    short ret = 0;
    static short state = 0;
    char errFormat[] = "\e[1;31m"; // Placeholder for color text
    /* Uses a static state machine to store the sate between runs */
    if(state == 0){
        if(CHANCE < CHANCE_TO_GET_CHKSUM_ERROR){
            printf("%sCheck sum error on %d\e[0m\n", errFormat, (int) data->SEQ);
            data->cksum  = (short) ingsoc_randomNr(120,3000);
            ret += 1;
        }
        /* Generate out of order
        * Its possible randomly both send data to jail and check out data form
        * jail. To do that we use an array of sort to mimic select  */
        if (CHANCE < CHANCE_TO_GET_OUT_ORDER){
            /* Send package to jail */
            _sendToJail(data);
            #ifdef ERROR_MESSAGE_IN_GENERATOR
            if(_sendToJail(data)) printf("sending a ingsoc struct %sto\e[0m jail", errFormat);
            else printf("could %snot\e[0m send ingsoc struct to jail because jail is full", errFormat);
            #endif
            ret += 2;
        }
        if(CHANCE < 20){
            /* Return a package from jail to sender and set state 1 */
            if(_numberInJail() > 0) ret+=4;
            state = 1;
        }
    } else if( state == 1){
        /* Package breaks out of jail and is send ed */
        if(number_of_inmates > 0) data = _returnFromJail();
        state = 0;
    }

    //printf("errorGenerator \e[033mEND\e[0m\n");
    return ret;
}

void ingsoc_writeMessage(int fileDescriptor, ingsoc* data, int length, struct sockaddr_in *host_info) {
    /* Ingsoc writeMessage is the function writes the data in the insoc
     * structure to the socket but first we calculate the check sum for the
     * gackage, that is done in the ingsoc cksum function */
    int nOfBytes = 0;
    size_t buffer_size = sizeof(ingsoc) + 10;
    char buff[buffer_size];
    char *buffer = buff;
    memset(buffer, 0, buffer_size);
    data->cksum = 0;
    /* First we set the check sum to zero before wi calculate the checksum */
    toSerial(data,buffer);
    data->cksum = ingsoc_cksum(buffer, buffer_size);
    /* Checkcsum is now calculated and set in the structure */
    short err = 0;

    /* Error generator that simulate errors
     * Read the error generator function to understand how that one works
     * But it returns a value that corisponds to a certain type of error. */

    err = errorGenerator(data);
    if( (err == 4) | (err == 4) | (err == 1+4)){
        /* Breakes a old data out of jail */
        ingsoc outofjail;
        errorGenerator(&outofjail);
        //printf("Breaking out of jail\n");
        newspeak(&outofjail);
        nOfBytes = sendto(fileDescriptor, &outofjail, length, 0, (struct sockaddr*)host_info,sizeof(*host_info));
    }
    if( (err == 2) | (err == 1+2) | (err == 2+4)){
        //printf("Didn \e[031mnot\e[0m send data beause it got detained in jail, err=%d\n", err);
        printf(".\n");
    }else{
        newspeak(data);
        nOfBytes = sendto(fileDescriptor, data, length, 0, (struct sockaddr*)host_info,sizeof(*host_info));
    }

    if(nOfBytes < 0){
        perror("writeMessage - Could \e[031mnot\e[0m WRITE data to socket\n");
#ifdef ERROR_MESSAGE_ON_NO_SOCKET
        /* Debugging message */
        char temp[4];
        temp[0] = (data->ACK) ? 't' : 'f';
        temp[1] = (data->FIN) ? 't' : 'f';
        temp[2] = (data->RES) ? 't' : 'f';
        temp[3] = (data->SYN) ? 't' : 'f';
        printf("Flags:%s", temp);
        printf(" ACKnr=%ld, SEQ=%ld, clientID=%ld, cksum=%d, length=%d", data->ACKnr,data->SEQ, data->clientID, data->cksum, data->length);
#endif
        //exit(EXIT_FAILURE);
    }
}
