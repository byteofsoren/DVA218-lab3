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

