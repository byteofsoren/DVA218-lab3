
#include "ingsoc.h"


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
    memcpy(buffer + counter, package->data, 255);
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

    fgets(msg, 1024, stdin);

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

    unsigned nOfBytes = sizeof(*host_info);
    int dataRead = 0, sentChSum = 0;
//    char buffer[MAXMSG];

    dataRead = recvfrom(fileDescriptor, data, MAXMSG, 0, (struct sockaddr *) host_info, &(nOfBytes));
    if(dataRead < 0){
        perror("readMessage - Could not READ data");
        //exit(EXIT_FAILURE);
        return -1;
    }
    sentChSum = data->cksum;
    data->cksum = 0;
    size_t buffer_size = sizeof(ingsoc) + 10;
    char buff[buffer_size];
    char *buffer = buff;
    memset(buffer, 0, buffer_size);

    toSerial(data,buffer);
    data->cksum = ingsoc_cksum(buffer, buffer_size);
    //printf("%d\n",data->cksum);
    if(data->cksum == sentChSum)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */

bool errorGenerator(int *fileDescriptor, ingsoc* data, struct sockaddr_in *host_info, struct sockaddr_in *host_info_cpy){
    /* A function to statistically create errors. The three first variables is
     * used for defining the chance of error between 0 and 100%, so 10 is 10%
     * chance to generate a error */
    short CHANCE_TO_GET_CHKSUM_ERROR = 10;
    short CHANCE_TO_GET_BAD_FD = 0;
    short CHANCE_TO_GET_WRONG_HOST_INFO = 10;
    /* In side the print statement below there is a control sequence that begins 
     * with e[0** that sequence is used to color the output */
    printf("errorGenerator \e[032mStart\e[0m\n");
    /* A random number generated by ingsoc_random is used to generate a number between
     * 0 and 100 then check if that number is higher or lower then the settings
     * abode */
    short chkerror = (short) ingsoc_randomNr(0,100);
    bool ret = false;
    char errFormat[] = "\e[1;31m";
    if(chkerror < CHANCE_TO_GET_CHKSUM_ERROR){
        printf("%sChec sum error\e[0m\n", errFormat);
        data->cksum  = 6543;
    }
    chkerror = (short) ingsoc_randomNr(0,100);
    if(chkerror < CHANCE_TO_GET_BAD_FD)
    {
        printf("%sChange fileDescriptor\e[0m\n", errFormat);
        *fileDescriptor = 10;
    }
    chkerror = (short) ingsoc_randomNr(0,100);
    if(chkerror < CHANCE_TO_GET_WRONG_HOST_INFO){
        /* Generate a error on the port information that is then stored in the
         * structure */
        printf("%sCreate error host_info\e[0m\n",errFormat);
        host_info_cpy = host_info;
        host_info_cpy->sin_port = 543;
        ret = true;
    }
    printf("errorGenerator \e[033mEND\e[0m\n");

    return ret;
}

void ingsoc_writeMessage(int fileDescriptor, ingsoc* data, int length, struct sockaddr_in *host_info) {
    /* Ingsoc writeMessage is the function writes the data in the insoc
     * structure to the socket but first we calculate the check sum for the
     * gackage, that is done in the ingsoc cksum function */
    int nOfBytes;
    size_t buffer_size = sizeof(ingsoc) + 10;
    char buff[buffer_size];
    char *buffer = buff;
    memset(buffer, 0, buffer_size);
    data->cksum = 0;
    toSerial(data,buffer);

    data->cksum = ingsoc_cksum(buffer, buffer_size);
    struct sockaddr_in *host_info_cpy;
    host_info_cpy = (struct sockaddr_in*) calloc(1, sizeof(struct sockaddr_in));
    bool err = false;
    /* Error generator tho simulate error */
    err = errorGenerator(&fileDescriptor, data, host_info, host_info_cpy);
    if (err){
        printf("ingsoc_writeMessage status: fileDescriptor=%d, cksum=%d, porT=%d\n", fileDescriptor, data->cksum, host_info_cpy->sin_port);
        nOfBytes = sendto(fileDescriptor, data, length, 0, (struct sockaddr*)host_info_cpy,sizeof(*host_info));
    } else {
        printf("ingsoc_writeMessage status: fileDescriptor=%d, cksum=%d, porT=%d\n", fileDescriptor, data->cksum, host_info->sin_port);
        nOfBytes = sendto(fileDescriptor, data, length, 0, (struct sockaddr*)host_info,sizeof(*host_info));
    }

    if(nOfBytes < 0){
        perror("writeMessage - Could not WRITE data to socket\n");
        //exit(EXIT_FAILURE);
    }
    free(host_info_cpy);
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
