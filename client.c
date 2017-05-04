#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

char HW_CONNECT[200] = "wlp1s0";
int FD_SOCKET;

void _initSocketAddress6(struct sockaddr_in6 *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin6_family = AF_INET6;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin6_port = htons(port);
  /* Get info about host. */
  name->sin6_scope_id=if_nametoindex(HW_CONNECT);
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

void _initSocketAddress(struct sockaddr_in *name, const char *hostName, unsigned short int port)
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

int _connect(const char *address)
{
    //char buffer[MAXMSG];
    //int nBytes = 0;
    int sock = 0;
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if(sock < 0){
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    FD_SOCKET = sock;
    struct sockaddr_in serverName;
    _initSocketAddress(&serverName, address, PORT);
    //int nBytes = sendto(sock,"Hej Fucktard", 13,0,(struct sockaddr*) &serverName,sizeof(serverName));
    //_writeMessage(sock, "Hello Hampus");
  /*
    nBytes = sendto(sock,"Hej Fucktard", 13,0,(struct sockaddr*) &serverName,sizeof(serverName));
    usleep(10000);
    int fucktard = sizeof(serverName);
    nBytes = recvfrom(sock,buffer,MAXMSG,0,(struct sockaddr *) &serverName ,&(fucktard));
    printf("%s\n",buffer);
  */

    short state = 0;
    bool running = 1;
    int counter = 10;
    int t;

    while(running){
        switch (state) {
            case 0:
                do{
                    ingsoc sSyn;
                    sSyn.clientID = getpid();
                    sSyn.ACK = false;
                    sSyn.FIN = false;
                    sSyn.RES = false;
                    sSyn.SEQ = 0;
                    sSyn.cksum = 0;
                    sSyn.length = 0;
                    sSyn.data = 0;
                    sSyn.SYN = true;

                    ingsoc_writeMessage(FD_SOCKET, &sSyn, sizeof(sSyn), &serverName);
                    printf("Client - [SYN sent]\n");

                    fd_set clientFD;
                    FD_ZERO(&clientFD);
                    FD_SET(FD_SOCKET, &clientFD);

                    struct timeval timer;
<<<<<<< HEAD
                    timer.tv_sec=10;
                    timer.tv_usec=5000;
                    int t = select(FD_SETSIZE, &clientFD, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(FD_SOCKET, &clientFD)) {
=======
                    timer.tv_sec = 10;
                    timer.tv_usec = 5000;

                    t = select(5, &clientFD, NULL, NULL, &timer);
                    if (t == -1)
                        perror("Client - [Select failed]\n");


                    if (FD_ISSET(FD_SOCKET, &clientFD))
                    {
>>>>>>> eb415bfaf7a138eb81f0fbb5613bcfb0d0bcc0a8
                        ingsoc rAck;
                        ingsoc_readMessage(FD_SOCKET, &rAck, &serverName);
                        FD_CLR(FD_SOCKET, &clientFD);

                        if (rAck.ACK)
                        {
                            printf("Client - [ACK received] attempt %d\n" ,10-counter);
                            state = 1;
                            break;
                        }else
                        {
                            printf("Client - [ACK corrupt]");
                            //Go to reject state <----------------
                            exit(EXIT_FAILURE);
                        }
                    }
                    else
                    {
                        printf("Timeout counter is now %d\n", counter);
                        counter--;
                        if(counter == 0)
                            exit(EXIT_FAILURE);
                    }
                }while(1);
                state = 1;
                break;
            case 1:
                // Send final ACK to server
                printf("Client - [Three-way Handshake successful]\n");
                running = 0;
                //break;
            case 2:
                //Sent reject
                break;
        }
    }
    return 0;
}

void client_main(char *addres)
{
    _connect(addres);
    //_writeMessage(sock, "Hello Hampus");
}
