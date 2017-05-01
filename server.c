
#define  blacklist "192.168.43.39"
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>infigo
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <net/if.h>
#include "server.h"

#define PORT 5555
#define MAXMSG 512

int make_Socket(unsigned short int port) {
    int sock;
    struct sockaddr_in6 name;

    sock = socket(PF_INET6, SOCK_STREAM, 0);     //UDP connection
    if(sock < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }

    name.sin6_family = AF_INET6;
    name.sin6_port = htons(port);
    name.sin6_addr=in6addr_any;
    name.sin6_scope_id= 3;
    //if_nametoindex("wlp3s0");
    if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("Could not bind a name to the socket\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("kakor...Fucking kakor\n");
    }
    return(sock);
}
void Socket_Main(int arg){
    int sock;
    int clientSocket;

/* Create a socket and set it up to accept connections */
    sock = make_Socket(PORT);
    /* Listen for connection requests from clients */
    if(listen(sock,1) < 0) {
        perror("Could not listen for connections\n");
        exit(EXIT_FAILURE);
    }
}

