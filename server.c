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


void Server_Main(int arg){
    int sock,i;
    struct sockaddr_in  clientInfo;
    //int clientSocket;
    char buffer[MAXMSG];
    int nOfBytes = 0;
    fd_set activeFdSet, readFdSet; /* Used by select */

/* Create a socket and set it up to accept connections */
    sock = make_Socket4(PORT);

    /* Initialize the set of active sockets */
    FD_ZERO(&activeFdSet);
    FD_SET(sock, &activeFdSet);
    printf("\n[waiting for connections...]\n");
    while(1) {
        readFdSet = activeFdSet;
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) {
            perror("Select failed\n");
            exit(EXIT_FAILURE);

        }
        if (FD_ISSET(sock, &readFdSet)) {

            printf("hejsan hoppsan\n");
            int fucktard = sizeof(clientInfo);
            nOfBytes = recvfrom(sock, buffer, MAXMSG, 0, (struct sockaddr *) &clientInfo, &(fucktard));
            //nOfBytes = read(i, buffer, MAXMSG);
            if (nOfBytes < 0) {
                printf("Did not reade any data from read()\n");
            } else {
                printf("%s\n", buffer);
                int Snucktard = sizeof(clientInfo);
                nOfBytes = sendto(sock, buffer, nOfBytes, 0, (struct sockaddr *) &clientInfo, Snucktard);
            }
        }
    }
}

