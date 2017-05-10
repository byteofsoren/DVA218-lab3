#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

char HWclient_connect[200] = "wlp1s0";
int GSOCKET;
struct sockaddr_in SERVER_NAME;

void client_init_socket_addres6(struct sockaddr_in6 *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin6_family = AF_INET6;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin6_port = htons(port);
  /* Get info about host. */
  name->sin6_scope_id=if_nametoindex(HWclient_connect);
  //name->sin6_scope_id=3;  // Wierles interface is 3 on most cases.
  name->sin6_flowinfo=0;
  //hostInfo = gethostbyname(hostName);  //Obsolite
  unsigned char buf[sizeof(struct in6_addr)];
  int t = inet_pton(AF_INET6, hostName, buf);
  if (t <= 0){
    printf("inet_pton genereated a1n error");
    exit(EXIT_FAILURE);
  }
  hostInfo = gethostbyaddr(buf, sizeof(buf), AF_INET6);
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin6_addr = *(struct in6_addr *)hostInfo->h_addr;
}


void client_init_socket_addres(struct sockaddr_in *name, const char *hostName, unsigned short int port)
{
    // A re implementation for iPV4
    struct hostent *hostInfo;
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    hostInfo = gethostbyname(hostName);
    if (hostInfo == NULL) {
        printf("Error gethostbyname");
        exit(EXIT_FAILURE);
    }
    name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}

int client_connect(fd_set *ActiveFdSet, const char *addres) {
    //char buffer[MAXMSG];
    //int nBytes = 0;
    GSOCKET = socket(PF_INET, SOCK_DGRAM, 0);
    if (GSOCKET < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    client_init_socket_addres(&SERVER_NAME, addres, PORT);
    FD_ZERO(ActiveFdSet);
    FD_SET(GSOCKET, ActiveFdSet);

    short state = 0;
    fd_set GFD_SET;
    bool running = 1;
    int i = 0, counter = 5;
    size_t ACK_NR = 0;
    int windowSize = ingsoc_randomNr(1, 5);
    ingsoc sSyn;
    //FD_ZERO(&GFD_SET);
    //FD_SET(GSOCKET, &GFD_SET);
    while (running) {
        switch (state) {
            case 0:

                //send syn

                ingsoc_init(&sSyn);
                sSyn.SYN = true;

                //_writeMessage(GSOCKET, (char*)&sSyn);
                ingsoc_seqnr(&sSyn);


                ingsoc_writeMessage(GSOCKET, &sSyn, sizeof(sSyn), &SERVER_NAME);
                while(counter > 0 && state == 0) {
                    struct timeval timer;
                    timer.tv_sec = 1;
                    GFD_SET = *ActiveFdSet;
                    int t = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(GSOCKET, &GFD_SET)) {
                        ingsoc rAck;
                        ingsoc_readMessage(GSOCKET, &rAck, &SERVER_NAME);

                        if (rAck.ACK == true && rAck.SYN == true && rAck.ACKnr == sSyn.SEQ) {

                            ACK_NR = rAck.SEQ;
                            int csum1 = rAck.cksum;
                            rAck.cksum = 0;
                            int csum = checkSum(&rAck, sizeof(rAck), 0);
                            printf("skickad %d, räknad %d\n", csum1, csum);
                            printf("ACK + SYN recived\n");
                            windowSize = rAck.length;
                            state = 1;
                        } else {
                            printf("!ACK + SYN recived\n");
                            exit(EXIT_FAILURE);
                        }
                        // Read from socket.
                        // om ACk -> state = 1;
                        // om ej ACK -> exit

                    } else {
                        printf("Time out counter is now %d\n", counter);
                        counter--;
                        if (counter == 0) exit(EXIT_FAILURE);
                    }
                }
                //Recive ack
                //or time out
                break;
            case 1:{
                ingsoc sACK;
                ingsoc_init(&sACK);
                ingsoc_seqnr(&sACK);
                sACK.ACK = true;
                sACK.ACKnr = ACK_NR;
                printf("Sendeing ACK_NR to server\n");
                ingsoc_writeMessage(GSOCKET, &sACK, sizeof(sACK), &SERVER_NAME);
                struct timeval timer;
                timer.tv_sec = 5;
                timer.tv_usec = 0;
                printf("Reading socket in final state\n");
                GFD_SET = *ActiveFdSet;
                int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                if(stemp == -1){
                   printf("Problem with select in sate 1");
                }

                if(FD_ISSET(GSOCKET, &GFD_SET)){
                    ingsoc rACK;
                    ingsoc_readMessage(GSOCKET, &rACK, &SERVER_NAME);
                    if(rACK.ACK == true && rACK.SYN == true && rACK.ACK == sSyn.SEQ){
                        printf("Recived ACK + SYN in final state\n");
                    }
                }else{
                    // time out exits the loop
                    running = 0;    // Stops the program
                    printf("No duplicate packets...continuing to sliding Windows\n");
                }

                break;
            }
        }
    }
    printf("--- END ---\n\tEnded 3 way handsaheke function\n");
    return 0;
}

int client_dis_connect(fd_set GFD_SET)
{
    /* This is the disconect functino */
    printf("--- INIT---\n\tIniting a client client_dis_connect\n");
    ingsoc sFin;
    ingsoc_init(&sFin);
    sFin.FIN = true;
    ingsoc_writeMessage(GSOCKET, &sFin, sizeof(sFin), &SERVER_NAME);
    struct timeval timer;
    timer.tv_sec = 10;
    timer.tv_usec = 0;
    printf("Waiting for fin + ack\n");
    FD_SET(GSOCKET, &GFD_SET);
    int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
    if(stemp == -1) perror("select");
    if (FD_ISSET(GSOCKET, &GFD_SET )) {
        // Reads message for server.
        ingsoc rAck;
        ingsoc_readMessage(GSOCKET, &rAck, &SERVER_NAME);
        if (rAck.ACK == true && rAck.FIN == true) {
            printf("Recived fin + ack");
            sFin.ACK = true;
            ingsoc_writeMessage(GSOCKET, &sFin, sizeof(sFin), &SERVER_NAME);
            // Do i need tto do any discconecting on UDP?
        }
    }
    // wait for FIN + ACK
    // Send ACK + FIN
    // close
    return 0;
}
void SWSend(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo){

    int state;
    int running = 1;
    ingsoc toWrite, toRead;
    fd_set readFdSet;
    struct timeval timer;

    ingsoc_init(&toWrite);
    ingsoc_init(&toRead);

    if(toWrite.data != 0)
        state = 0;

    do {
        switch (state) {

            case 0:
                printf("Message:\n");
                char *buffer = malloc(512);
                input(buffer);
                state = 1;
                break;
            case 1:
                /* Adding the message to the package */
                strcpy(toWrite.data, buffer);
                /* Generating SEQnr */
                ingsoc_seqnr(&toWrite);
                free(buffer);

                state = 2;
                break;
            /* Case 2 - Send a package */
            case 2:
                /* Sending the package to server */
                ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                printf("Client - Package sent, waiting for ACK\n");
                state = 3;
                break;
            /* Case 1 - Waiting for ACK on package */
            case 3:
                readFdSet = *activeFdSet;
                /* Looking for changes in FD */
                timer.tv_sec = 5;
                if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                    perror("Client - Select failure");
                /*  */
                if(FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads the package from client */
                    ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo);
                    /* If it receives the SYN it proceeds to the next state */
                    if (toRead.ACK == true && toRead.ACKnr == toWrite.SEQ) {
                        printf("Client - ACK received\n");
                        /* Ready to send a new package */
                        state = 0;
                    }
                    else
                    {
                        printf("Client - ACK Corrupt, resending\n");
                        state = 2;
                    }
                }
                else {
                    printf("Client - ACK Timeout, resending.\n");
                    state = 2;
                }
                break;
        }
    }while(running == 1);
}
void client_main(char *addres)
{
    fd_set GFD_SET;
    client_connect(&GFD_SET, addres);
    SWSend(&GSOCKET, &GFD_SET, &SERVER_NAME);
    printf("--Initing client_dis_connect---\n");
    client_dis_connect(GFD_SET);
}
