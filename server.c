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

    /* Create a socket. */
    sock = socket(PF_INET, SOCK_DGRAM, 0);     //TCP connection
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

void connection(int *sock, fd_set *activeFdSet, struct sockaddr_in *clientInfo) {
    int nOfBytes = 0;
    struct timeval timeout;
    int state = 0;
    int n = 0;
    int t;
    int running = 1;
    ingsoc rACK, sACK;
    rACK.SYN = false;
    rACK.ACK = false;
    sACK.ACK = false;

    while (1) {
        timeout.tv_usec = 50000;
        timeout.tv_sec = 10;
        fd_set readFdSet = *activeFdSet;

        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
            perror("Server - [Select Failed]\n");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(*sock, &readFdSet)) {
            printf("Server - [Starting three-way handshake]\n");

            switch (state) {

                /* Waiting for SYN from client */
                case 0:
                    do {
                        ingsoc_readMessage(*sock, &rACK, clientInfo);

                        if (rACK.SYN == true) {
                            printf("Server - [SYN received] attempt %d\n", n + 1);
                            state = 1;
                            n = 0;
                            break;
                        } else n++;
                    } while (n <= 3);
                    break;

                case 1: //Send ACK + SEQ then wait for final ACK
                    do {
                        sACK.ACK = true;
                        sACK.SEQ = 13;
                        /*Sending ACk and SEQ */
                        ingsoc_writeMessage(*sock, &sACK, sizeof(sACK), clientInfo);
                        printf("Server - [ACK and SEQ sent]\n");
                        /* Waiting for final ACK */
                        ingsoc_readMessage(*sock, &rACK, clientInfo);

                        if (rACK.ACK == true) {
                            printf("Server - [Final ACK received] attempt %d\n", n);
                            state = 2;
                            n = 0;
                            break;
                        } else n++;
                    } while (n <= 3);
                    break;
                case 2:
                    printf("Server - [Three-way handshake successful]\n");
                    running = 0;
                    break;
            }

        }
    }
}

void Server_Main(int arg){
    int sock;
    struct sockaddr_in  clientInfo;
    int nOfBytes = 0;
    char buffer[MAXMSG];
    fd_set readFdSet, activeFdSet; /* Used by select */
    sock = make_Socket4(PORT);
    FD_ZERO(&activeFdSet);
    FD_SET(sock,&activeFdSet);
/* Create a socket and set it up to accept connections */


    /* Initialize the set of active sockets */

    printf("\n[waiting for connections...]\n");

    connection(&sock,&activeFdSet,&clientInfo);

}

