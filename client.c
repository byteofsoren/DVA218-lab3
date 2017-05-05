#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

char HW_CONNECT[200] = "wlp1s0";
int FD_SOCKET;
struct sockaddr_in serverName;
fd_set _sock;
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

void _waitfor_socket()
{
    FD_ZERO(&_sock);
    FD_SET(FD_SOCKET, &_sock);
    struct timeval timer;
    timer.tv_sec=10;
    timer.tv_usec=5000;
    int t = 0;
    t = select(FD_SETSIZE, &_sock, NULL, NULL, &timer);
    if (t == -1) {
        printf("Error in select");
        exit(EXIT_FAILURE);
    }
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

int _connect(const char *addres) {
    //char buffer[MAXMSG];
    //int nBytes = 0;
    int sock = 0;
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Could not create a socet\n");
        exit(EXIT_FAILURE);
    }
    FD_SOCKET = sock;
    _initSocketAddress(&serverName, addres, PORT);
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
    while (running) {
        switch (state) {
            case 0:
                do {
                    //send syn
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
                    //_writeMessage(FD_SOCKET, (char*)&sSyn);

                    ingsoc_writeMessage(FD_SOCKET, &sSyn, sizeof(sSyn), &serverName);

                    fd_set sock;
                    FD_ZERO(&sock);
                    FD_SET(FD_SOCKET, &sock);
                    struct timeval timer;
                    timer.tv_sec = 10;
                    int t = select(FD_SETSIZE, &sock, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(FD_SOCKET, &sock)) {
                        ingsoc rAck;
                        ingsoc_readMessage(FD_SOCKET, &rAck, &serverName);
                        FD_CLR(FD_SOCKET, &sock);
                        if (rAck.ACK) {
                            printf("ACK reseved");
                            state = 1;
                        } else {
                            printf("!ACK recived");
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
                    //Recive ack
                    //or time out
                } while (1);
            case 1:
                break;
        }
    }
}

int _disConect()
{
    // Send FIN
    ingsoc sFin;
    ingsoc_init(&sFin);
    sFin.FIN = true;
    int running = 1;
    ingsoc_writeMessage(FD_SOCKET, &sFin, sizeof(sFin), &serverName);
    do{
        _waitfor_socket();
        if (FD_ISSET(FD_SOCKET, &_sock )) {
            // Reads message for server.
            ingsoc rAck;
            ingsoc_readMessage(FD_SOCKET, &rAck, &serverName);
            if (rAck.ACK == true && rAck.FIN == true) {
                running = 0;
                sFin.ACK = true;
                ingsoc_writeMessage(FD_SOCKET, &sFin, sizeof(sFin), &serverName);
                // Do i need tto do any discconecting on UDP?
            }
        }
    }while(running);
    // wait for FIN + ACK
    // Send ACK + FIN
    // close
    return 0;
}

void client_main(char *addres)
{
    _connect(addres);
    //_writeMessage(sock, "Hello Hampus");
    _disConect();
}
