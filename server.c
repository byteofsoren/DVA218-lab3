#include "ingsoc.h"

#define PORT 5555
#define MAXMSG 512
size_t LatestRecSeq;
int make_Socket6(unsigned short int port) {
    int sock;
    struct sockaddr_in6 name;


    sock = socket(PF_INET6, SOCK_DGRAM, 0);     //UDP connection
    if (sock < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }

    name.sin6_family = AF_INET6;
    name.sin6_port = htons(port);
    name.sin6_addr = in6addr_any;
    name.sin6_scope_id = 3;
    //if_nametoindex("wlp3s0");e an ipv6 gateway. Could you post the output of ip -6 route?
    if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
        perror("Could not bind a name to the socket\n");
        exit(EXIT_FAILURE);
    } else {
        printf("kakor...Fucking kakor\n");
    }
    return (sock);
}
int make_Socket4(unsigned short int port) {
    int sock;
    struct sockaddr_in name;

    /* Create a socket. TCP connection */
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Assign an address to the socket by calling bind. */
    if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("Could not bind a name to the socket\n");
        exit(EXIT_FAILURE);
    }
    return(sock);
}
/* server_disconnect - This function is initialized when the server receives a FIN (disconnect request)
 * from the client*/
int server_disconnect(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo)
{

    ingsoc toRead, toWrite;
    int n = 0;
    struct timeval timer;
    fd_set readFdSet;

    ingsoc_init(&toRead);
    ingsoc_init(&toWrite);

    toWrite.ACK = true;
    toWrite.FIN = true;


    do {
        readFdSet = *activeFdSet;

        ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
        printf("Server - FIN+ACK sent to client\n");

        timer.tv_sec = 5;
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
            perror("Server - Select failure");

        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == -1)
            {
                toRead.ACK = false;
                toRead.ACKnr = 0;
            }
                if (toRead.ACK == true) {
                    printf("Server - FIN ACK received, disconnecting.\n");
                    n = 4;
                } else {
                    printf("Server - timeout %d", n + 1);
                    n++;
                }

        }
    }while(n <= 3);

    return 0;
}
/* Threeway - This is the server part of the threeway handshake
 * Inputs:
 * fileDescriptor - Socket handle
 * activeFdSet - List of active FDs (which is only one, port 5555)
 * hostInfo - struct for handling internet addresses */
int Threeway(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {
    
    ingsoc toWrite, toRead;
    int state = 0;
    int running = 1;
    int n = 0, windowSize = ingsoc_randomNr(3,20);
    struct timeval timer;


    fd_set readFdSet;

    do {
        switch (state) {
            /* case 0 - Waiting for SYN from client */
            case 0:
                readFdSet = *activeFdSet;
                /* Looking for changes in FD */
                if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0)
                    perror("Server - Select failure");
                /*  */
                if(FD_ISSET(*fileDescriptor, &readFdSet))
                {
                    /* Reads the package from client */
                    if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0)
                    {
                        /* If it receives the SYN it proceeds to the next state */

                        if (toRead.SYN == true) {
                            printf("Server - SYN received\n");
                            if (toRead.length < windowSize) {
                                windowSize = toRead.length;
                            }
                            state = 1;
                        }
                    }
                }
                break;
            /* case 1 - Send SYN + ACK to client and SEQ nr, then wait for final ACK */
            case 1:

                ingsoc_init(&toWrite);
                toWrite.ACK = true;
                toWrite.SYN = true;
                toWrite.length = windowSize;
                ingsoc_seqnr(&toWrite);
                toWrite.ACKnr = toRead.SEQ;
                while(state == 1) {
                    //toWrite.cksum = checkSum(&toWrite, sizeof(toWrite), 0);
                    //printf("checksum: %d\n", toWrite.cksum);

                    do {
                        /* Sends the SYN+ACK package to client */
                        ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                        printf("Server - ACK + SYN sent\n");
                        timer.tv_sec = 20;
                        timer.tv_usec = 5000;
                        readFdSet = *activeFdSet;
                        /* Looks for changes in FD */
                        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                            perror("Server - Select failure");

                        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                            if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == -1)
                            {
                                ingsoc_init(&toRead);
                            }
                            /* After sending SYN+ACK and receving the final ack from client
                             * it will proceed to the next state, which is the final state */
                            if (toRead.ACK == true && toRead.ACKnr == toWrite.SEQ) {
                                printf("Server - final ACK received\n");
                                state = 2;
                            } else {

                                /* If for some reason the package is lost or something else is
                                 * received, it will add 1 to a counter and resend the SYN+ACK
                                 * package, after n timeouts it will exit this state */

                                printf("Server - ACK not received, attempt: %d", n + 1);
                                n++;
                            }
                        } else {
                            printf("Timeout\n");
                        }
                    } while (state == 1 && n <= 3);
                }

                break;

            case 2:
                /* This is the final state, once we get here the client and server is
                 * connected and the threeway handshake has been successful, from here
                 * we will proceed with the sliding window protocol. */
                LatestRecSeq = toRead.SEQ;
                printf("Server - Three-way handshake successful\n");
                running = 0;
                break;
        }
    }while(running == 1);
    return windowSize;
}
void SWRecv(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo, int windowSize){

    ingsoc toWrite, toRead;
    int state = 0;
    size_t toACK = 0;
    size_t startPos = 0;
    size_t running = 1;
    size_t NrInWindow = 0;
    size_t PlaceInWindow = 0;
    //size_t endPos = startPos + windowSize;
    int i;
    fd_set readFdSet;
    int offset = 0;
    //ingsoc window[windowSize];
    char *message = malloc(512);
    int PlaceInMessage = 0;
    ingsoc *Window = malloc(windowSize * sizeof(ingsoc));
    bool *populated = malloc(windowSize * sizeof(bool));
    for(i = 0; i < windowSize; i++)
    {
        populated[i] = false;
    }
    ingsoc_init(&toRead);
    ingsoc_init(&toWrite);

    do {
        switch (state) {

            case 0:

                readFdSet = *activeFdSet;
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0)
                    perror("Server - Select failure");

                if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads the package from client */
                    if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0)
                    {
                        if (toRead.FIN == true)
                            state = 8;
                        else {
                            for (i = 0; i < windowSize; i++) {
                                if (toRead.SEQ == Window[i].SEQ && populated[i] == true) {
                                    state = 1;
                                }
                            }
                            if (state != 1) {

                                if (LatestRecSeq - toRead.SEQ < 1000) {
                                    state = 1;
                                }
                                else if (toRead.SEQ - LatestRecSeq <= windowSize - NrInWindow) {
                                    state = 1;
                                    toACK = PlaceInWindow + (toRead.SEQ - LatestRecSeq - 1);
                                    if ((int) toACK >=  windowSize) {
                                        toACK -= windowSize;
                                    }
                                    Window[toACK] = toRead;
                                    populated[toACK] = true;
                                    if (toACK == PlaceInWindow) {
                                        LatestRecSeq = toRead.SEQ;
                                    } else {
                                        offset++;
                                    }
                                    NrInWindow++;
                                }
                            }
                        }
                    }
                }
                break;

            case 1:

                printf("Server - Package %ld received, SEQnr: %d\n", startPos, (int)toRead.SEQ);
                if (toACK == PlaceInWindow && Window[PlaceInWindow].ACK == false)
                {
                    for(i = 0; i < Window[PlaceInWindow].length; i++)
                    {
                        message[PlaceInMessage] = Window[PlaceInWindow].data[i];
                        PlaceInMessage++;
                    }
                    populated[PlaceInWindow] = false;
                    NrInWindow--;
                    PlaceInWindow++;
                    if((int) PlaceInWindow >= windowSize)
                    {
                        PlaceInWindow = 0;
                    }

                    while(populated[PlaceInWindow] == true && offset > 0)
                    {
                        populated[PlaceInWindow] = false;
                        LatestRecSeq = Window[PlaceInWindow].SEQ;
                        for(i = 0; i < Window[PlaceInWindow].length; i++)
                        {
                            message[PlaceInMessage] = Window[PlaceInWindow].data[i];
                            PlaceInMessage++;
                        }
                        PlaceInWindow++;
                        NrInWindow--;
                        if((int)PlaceInWindow >= windowSize)
                        {
                            PlaceInWindow = 0;
                        }
                        offset--;
                    }

                }
                Window[toACK].ACK = true;
                ingsoc_init(&toWrite);
                ingsoc_seqnr(&toWrite);
                toWrite.ACK = true;
                toWrite.ACKnr = toRead.SEQ;
                ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                printf("Sending ACK on %d\n", (int) toWrite.ACKnr);
                state = 0;
                break;
            case 8:
                message[PlaceInMessage] = '\0';
                /* Writes out message with green text \e[032m */
                printf("--Message was--\n[\e[032m%s \e[0m]\n", message);
                printf("send fin + Ackn\n");
                running = 0;
                free(message);
                free(populated);
                free(Window);
                break;
        }

    }while(running == 1);
}
void Server_Main(int arg){
    printf("arg=%d\n",arg);
    int fileDescriptor;
    struct sockaddr_in  hostInfo;
    int windowSize = 0;
    // int nOfBytes = 0;
    //char buffer[MAXMSG];
    //fd_set readFdSet;
    fd_set activeFdSet; /* Used by select */
    fileDescriptor = make_Socket4(PORT);
    FD_ZERO(&activeFdSet);
    FD_SET(fileDescriptor,&activeFdSet);
/* Create a socket and set it up to accept connections */

    /* Initialize the set of active sockets */

    //server_disconnect(&sock, &activeFdSet, &hostInfo);

    printf("\n[waiting for connections...]\n");

    windowSize = Threeway(&fileDescriptor, &activeFdSet, &hostInfo);
    printf("Window size if %d\n", windowSize);
    SWRecv(&fileDescriptor, &activeFdSet, &hostInfo, windowSize);
    server_disconnect(&fileDescriptor, &activeFdSet, &hostInfo);

}

