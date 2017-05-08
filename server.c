#include "ingsoc.h"

#define PORT 5555
#define MAXMSG 512

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
/* Threeway - This is the server part of the threeway handshake
 * Inputs:
 * fileDescriptor - Socket handle
 * activeFdSet - List of active FDs (which is only one, port 5555)
 * hostInfo - struct for handling internet addresses */
void Threeway(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {

    printf("Hej nu är det dags för connect!\n");

    ingsoc toWrite, toRead;
    int state = 0;
    int running = 1;
    int n = 0;
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
                    ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo);
                    /* If it receives the SYN it proceeds to the next state */
                    if(toRead.SYN == true)
                    {
                        printf("Server - SYN received\n");
                        state = 1;
                    }
                }
                break;
            /* case 1 - Send SYN + ACK to client and SEQ nr, then wait for final ACK */
            case 1:

                ingsoc_init(&toWrite);
                toWrite.ACK = true;
                toWrite.SYN = true;
                ingsoc_seqnr(&toWrite);
                toWrite.ACKnr = toRead.SEQ;


                do {
                    /* Sends the SYN+ACK package to client */
                    ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                    printf("Server - ACK + SYN sent\n");
                    timer.tv_sec = 20;
                    readFdSet = *activeFdSet;
                    /* Looks for changes in FD */
                    if(select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                        perror("Server - Select failure");

                    if(FD_ISSET(*fileDescriptor, &readFdSet))
                    {
                        ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo);
                        /* After sending SYN+ACK and receving the final ack from client
                         * it will proceed to the next state, which is the final state */
                        if(toRead.ACK == true)
                        {
                            printf("Server - final ACK received\n");
                            state = 2;
                            break;
                        }
                        /* If for some reason the package is lost or something else is
                         * received, it will add 1 to a counter and resend the SYN+ACK
                         * package, after n timeouts it will exit this state */
                        else if(toRead.ACK != true) {
                            printf("Server - ACK not received, attempt: %d", n + 1);
                            n++;
                        }
                    }
                    else{
                        printf("Timeout\n");
                    }
                }while(n <= 5);
                break;

            case 2:
                /* This is the final state, once we get here the client and server is
                 * connected and the threeway handshake has been successful, from here
                 * we will proceed with the sliding window protocol. */
                printf("Server - Three-way handshake successful\n");
                running = 0;
                break;
        }
    }while(running == 1);
    //SlidingWindowProtocol();
}
/* disconnect - This function is initialized when the server receives a FIN (disconnect request)
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

        ingsoc_writeMessage(fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
        printf("Server - FIN+ACK sent\n");

        timer.tv_sec = 5;
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
            perror("Server - Select failure");

        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            ingsoc_readMessage(fileDescriptor, &toRead, hostInfo);

            if (toRead.ACK == true)
            {
                printf("Server - FIN ACK received, disconnecting.\n");
            }
            else
            {
                printf("Server - timeout %d", n + 1);
                n++;
            }
        }
    }while(n <= 3);

}

void Server_Main(int arg){
    int sock;
    struct sockaddr_in  hostInfo;
    int nOfBytes = 0;
    char buffer[MAXMSG];
    fd_set readFdSet, activeFdSet; /* Used by select */
    sock = make_Socket4(PORT);
    FD_ZERO(&activeFdSet);
    FD_SET(sock,&activeFdSet);
/* Create a socket and set it up to accept connections */

    /* Initialize the set of active sockets */

    //server_disconnect(&sock, &activeFdSet, &hostInfo);

    printf("\n[waiting for connections...]\n");

    Threeway(&sock, &activeFdSet, &hostInfo);

}

