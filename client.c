#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

char HWclient_connect[200] = "wlp1s0";

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

int client_connect(int *GSOCKET, fd_set *ActiveFdSet, const char *addres) {
    //char buffer[MAXMSG];
    //int nBytes = 0;
    *GSOCKET = socket(PF_INET, SOCK_DGRAM, 0);
    if (*GSOCKET < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    client_init_socket_addres(&SERVER_NAME, addres, PORT);
    FD_ZERO(ActiveFdSet);
    FD_SET(*GSOCKET, ActiveFdSet);

    short state = 0;
    fd_set GFD_SET;
    bool running = 1;
    int i = 0, counter = 5;
    size_t ACK_NR = 0;
    int windowSize = ingsoc_randomNr(1, 5);
    ingsoc sSyn;

    while (running) {
        switch (state) {
            case 0:
                ingsoc_init(&sSyn);
                sSyn.SYN = true;
                sSyn.length = windowSize;

                ingsoc_seqnr(&sSyn);

                ingsoc_writeMessage(*GSOCKET, &sSyn, sizeof(sSyn), &SERVER_NAME);
                while(counter > 0 && state == 0) {
                    struct timeval timer;
                    timer.tv_sec = 1;
                    GFD_SET = *ActiveFdSet;
                    int t = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(*GSOCKET, &GFD_SET)) {
                        ingsoc rAck;
                        ingsoc_readMessage(*GSOCKET, &rAck, &SERVER_NAME);

                        if (rAck.ACK == true && rAck.SYN == true && rAck.ACKnr == sSyn.SEQ) {

                            ACK_NR = rAck.SEQ;
                            windowSize = rAck.length;
                            state = 1;
                        } else {
                            printf("!ACK + SYN recived\n");
                            exit(EXIT_FAILURE);
                        }
                    } else {
                        printf("Time out counter is now %d\n", counter);
                        counter--;
                        if (counter == 0) exit(EXIT_FAILURE);
                    }
                }
                break;
            case 1:{
                ingsoc sACK;
                ingsoc_init(&sACK);
                ingsoc_seqnr(&sACK);
                sACK.ACK = true;
                sACK.ACKnr = ACK_NR;

                ingsoc_writeMessage(*GSOCKET, &sACK, sizeof(sACK), &SERVER_NAME);
                printf("Client - ACK sent\n");

                struct timeval timer;
                timer.tv_sec = 5;

                printf("Client - Reading socket in final state\n");
                GFD_SET = *ActiveFdSet;

                int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                if(stemp == -1){
                   perror("Client - Select failed\n");
                }

                if(FD_ISSET(*GSOCKET, &GFD_SET)){
                    ingsoc rACK;
                    ingsoc_readMessage(*GSOCKET, &rACK, &SERVER_NAME);
                    if(rACK.ACK == true && rACK.SYN == true && rACK.ACK == sSyn.SEQ){
                        printf("Client - Final ACK received\n");
                    }
                }else{
                    running = 0;
                }

                break;
            }
        }
    }
    printf("Client - Three-way handshake successful\n");
    return windowSize;
}

int client_dis_connect(int *GSOCKET, fd_set GFD_SET)
{
    /* This is the disconect functino */
    printf("Client - Disconnecting\n");

    ingsoc sFin;
    ingsoc_init(&sFin);
    sFin.FIN = true;

    ingsoc_writeMessage(*GSOCKET, &sFin, sizeof(sFin), &SERVER_NAME);

    struct timeval timer;
    timer.tv_sec = 10;
    timer.tv_usec = 0;

    FD_SET(*GSOCKET, &GFD_SET);
    int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
    if(stemp == -1) perror("Client - Select failed");
    if (FD_ISSET(*GSOCKET, &GFD_SET )) {
        // Reads message for server.
        ingsoc rAck;
        ingsoc_readMessage(*GSOCKET, &rAck, &SERVER_NAME);
        if (rAck.ACK == true && rAck.FIN == true) {
            printf("Recived fin + ack");
            sFin.ACK = true;
            ingsoc_writeMessage(*GSOCKET, &sFin, sizeof(sFin), &SERVER_NAME);
        }
    }
    return 0;
}
void SWSend(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo, int windowSize){

    int state;
    int running = 1;
    ingsoc toWrite, toRead;
    fd_set readFdSet;
    struct timeval timer;
    int n = 0;
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
                //strcpy(toWrite.data, buffer[n]);
                toWrite.data[0] = buffer[n];
                toWrite.data[1] = '\0';
                /* Generating SEQnr */
                ingsoc_seqnr(&toWrite);
                if(buffer[n] == '\0')
                {
                    state = 4;
                }
                else
                {
                    state = 2;
                }
                n++;
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
                        state = 1;
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
            case 4:
                printf("Client - End of message\n");

                ingsoc_seqnr(&toWrite);
                toWrite.FIN = true;
                ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                running = 0;
                break;
        }
    }while(running == 1);
}
void client_main(char *addres)
{
    int GSOCKET, windowSize = 0;
    fd_set GFD_SET;
    windowSize = client_connect(&GSOCKET, &GFD_SET, addres);
    SWSend(&GSOCKET, &GFD_SET, &SERVER_NAME, windowSize);
    printf("--Initing client_dis_connect---\n");
    client_dis_connect(&GSOCKET, GFD_SET);
}
