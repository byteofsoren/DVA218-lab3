#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 1024

char HWclient_connect[200] = "wlp1s0";


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

int client_connect(int *GSOCKET, fd_set *ActiveFdSet, const char *addres, struct sockaddr_in *SERVER_NAME) {
    //char buffer[MAXMSG];
    //int nBytes = 0;
    *GSOCKET = socket(PF_INET, SOCK_DGRAM, 0);
    if (*GSOCKET < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    client_init_socket_addres(SERVER_NAME, addres, PORT);
    FD_ZERO(ActiveFdSet);
    FD_SET(*GSOCKET, ActiveFdSet);

    short state = 0;
    fd_set GFD_SET;
    bool running = 1;
    int  counter = 5;
    size_t ACK_NR = 0;
    int windowSize = ingsoc_randomNr(3, 20);
    ingsoc sSyn;
    //FD_ZERO(&GFD_SET);
    //FD_SET(*GSOCKET, &GFD_SET);
    while (running) {
        switch (state) {
            case 0:
                /* Starting the 3 way handshake */
                ingsoc_init(&sSyn);
                sSyn.SYN = true;
                sSyn.length = windowSize;
                //_writeMessage(*GSOCKET, (char*)&sSyn);
                ingsoc_seqnr(&sSyn);


                while(counter > 0 && state == 0) {
                    struct timeval timer;
                    ingsoc_writeMessage(*GSOCKET, &sSyn, sizeof(sSyn), SERVER_NAME);
                    timer.tv_sec = 1;
                    timer.tv_usec = 0;
                    GFD_SET = *ActiveFdSet;
                    int t = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(*GSOCKET, &GFD_SET)) {
                        ingsoc rAck;

                        if(ingsoc_readMessage(*GSOCKET, &rAck, SERVER_NAME) == 0)
                        {
                            if (rAck.ACK == true && rAck.SYN == true && rAck.ACKnr == sSyn.SEQ) {

                                ACK_NR = rAck.SEQ;
                                windowSize = rAck.length;
                                state = 1;
                            } else {
                                printf("!ACK + SYN recived\n");
                                //exit(EXIT_FAILURE);
                            }
                            // Read from socket.
                            // om ACk -> state = 1;
                            // om ej ACK -> exit

                        }
                    } else {
                        printf("\n------------------\nTime out counter is now \e[38;5;162m%d\e[0m\n", counter);
                        counter--;
                        state = 0;
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
                printf("Sending ACK_NR to server\n");
                ingsoc_writeMessage(*GSOCKET, &sACK, sizeof(sACK), SERVER_NAME);
                struct timeval timer;
                timer.tv_sec = 5;
                timer.tv_usec = 0;
                printf("Reading socket in final state\n");
                GFD_SET = *ActiveFdSet;
                int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                if(stemp == -1){
                   printf("Problem with select in sate 1");
                }

                if(FD_ISSET(*GSOCKET, &GFD_SET)){
                    ingsoc rACK;
                    ingsoc_readMessage(*GSOCKET, &rACK, SERVER_NAME);
                    if(rACK.ACK == true && rACK.SYN == true && rACK.ACK == sSyn.SEQ){
                        printf("Received ACK + SYN in final state\n");
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
    printf("--- END ---\n\tEnded 3 way handshake function\n");
    return windowSize;
}

int client_dis_connect(int *GSOCKET, fd_set GFD_SET, struct sockaddr_in *SERVER_NAME)
{
    /* This is the disconnect function */
    printf("Starting client client_dis_connect\n");
    int counter = 3;
    ingsoc sFin;
    ingsoc rAck;
    ingsoc_init(&sFin);
    ingsoc_seqnr(&sFin);
    sFin.FIN = true;
    usleep(40);
    while(counter >= 0) {
        ingsoc_writeMessage(*GSOCKET, &sFin, sizeof(sFin), SERVER_NAME);
        printf("Sent fin with SEQ: %d\n", (int)sFin.SEQ);
        struct timeval timer;
        timer.tv_sec = 10;
        timer.tv_usec = 0;
        printf("Waiting for fin + ack\n");

        int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
        if (stemp == -1) perror("select");
        if (FD_ISSET(*GSOCKET, &GFD_SET))
        {
            // Reads message for server.
            if(ingsoc_readMessage(*GSOCKET, &rAck, SERVER_NAME) == 0)
            {
                if (rAck.ACK == true && rAck.FIN == true && rAck.ACKnr == sFin.SEQ) {
                    printf("Recived fin + ack for %d\n", (int) rAck.ACKnr);
                    ingsoc_init(&sFin);
                    ingsoc_seqnr(&sFin);
                    sFin.ACK = true;
                    sFin.ACKnr = rAck.SEQ;
                    ingsoc_writeMessage(*GSOCKET, &sFin, sizeof(sFin), SERVER_NAME);
                    return 0;
                }
            }
        }
        counter--;
    }
    // wait for FIN + ACK
    // Send ACK + FIN
    // close
    return 0;
}
void SWSend(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo, int windowSize) {

    ingsoc toWrite, toRead;//window[windowSize];
    ingsoc *queue = malloc(windowSize * sizeof(ingsoc));
    clock_t *sent = malloc(windowSize * sizeof(clock_t));
    int state = 0;
    int i,t = 0, nOfPack, PackToResend;
    int running = 1;
    int PlaceInWindow = 0;      //where in the windows we are
    int PlaceForAck = 0;
    int length = 0;
    int NrInWindow = 0;     //how many packages there is in the window
    int PlaceInMessage = 0;     //where in the string to be sent we are
    //int tmpPos;
    char *buffer = malloc(1024);
    fd_set readFdSet;
    struct timeval timer;
    bool *populated = malloc(windowSize * sizeof(bool));
    for (i = 0; i < windowSize; i++)
    {
        populated[i] = false;
    }

    printf("Client - Message to send: \n");
    input(buffer);
    length = (int)strlen(buffer);

    ingsoc_init(&toWrite);
    ingsoc_init(&toRead);

    /*for (i = 0; i < nOfPack; i++) {

    }*/

    do {
        for (i = 0; i < windowSize; i++)
        {
            if((clock() - sent[i]) > 10000 && populated[i] == true && queue[i].ACK == false)
            {
                state = 3;
                PackToResend = i;
                i = windowSize;
            }

        }
        timer.tv_usec = 1;
        timer.tv_sec = 0;
        readFdSet = *activeFdSet;
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
            perror("Client - Select failure");
        /*  */
        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            if (state != 1 && state != 3) {
                state = 2;
            }
        }
        switch (state) {
            case 0:
                if(NrInWindow < windowSize && populated[PlaceInWindow] == false)
                {
                    state = 1;
                    if(buffer[PlaceInMessage] == '\0' && NrInWindow == 0)
                    {
                        state = 4;
                    }
                    else {
                        if (length - PlaceInMessage < 3) {
                            toWrite.length = (length - PlaceInMessage);
                        } else {
                            toWrite.length = ingsoc_randomNr(1, 3);
                        }
                        for (i = 0; i < toWrite.length; i++) {
                            toWrite.data[i] = buffer[PlaceInMessage];
                            PlaceInMessage++;
                        }

                        toWrite.data[toWrite.length] = '\0';
                        ingsoc_seqnr(&toWrite);
                        queue[PlaceInWindow] = toWrite;
                        sent[PlaceInWindow] = clock();
                        populated[PlaceInWindow] = true;
                        NrInWindow++;
                    }
                }
                else {
                    timer.tv_usec = 100;
                    timer.tv_sec = 0;
                    readFdSet = *activeFdSet;

                    if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                        perror("Client - Select failure");
                    /*  */
                    if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                        state = 2;
                    }

                }

                break;

            case 1:
                ingsoc_writeMessage(*fileDescriptor, &queue[PlaceInWindow], sizeof(ingsoc), hostInfo);
                printf("Client - Package %d sent, SEQ nr: %d\n", PlaceInMessage, (int) (queue[PlaceInWindow]).SEQ);
                sent[PlaceInWindow] = clock();
                PlaceInWindow++;
                if (PlaceInWindow >= windowSize) {
                    PlaceInWindow = 0;
                }

                state = 0;
                break;



            case 2:
                readFdSet = *activeFdSet;
                /* Looking for changes in FD */
                timer.tv_sec = 5;
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                    perror("Client - Select failure");

                if(FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads package from client */
                    if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0)
                    {
                        if(toRead.ACK == true && toRead.ACKnr == queue[PlaceForAck].SEQ)
                        {
                            NrInWindow--;
                            populated[PlaceForAck] = false;
                            PlaceForAck++;
                            if(PlaceForAck >= windowSize)
                            {
                                PlaceForAck = 0;
                            }
                            printf("ACK recieved: %d\n", (int) toRead.ACKnr);

                            while(queue[PlaceForAck].ACK == true && populated[PlaceForAck] == true)
                            {
                                queue[PlaceForAck].ACK = false;
                                NrInWindow--;
                                populated[PlaceForAck] = false;
                                PlaceForAck++;
                                if(PlaceForAck >= windowSize)
                                {
                                    PlaceForAck = 0;
                                }
                            }
                        }
                        else
                        {
                            t = PlaceForAck + 1;
                            if(t >= windowSize)
                            {
                                t = 0;
                            }
                            while(t != PlaceForAck)
                            {
                                if(toRead.ACKnr == queue[t].SEQ)
                                {
                                    queue[t].ACK = true;
                                    t = PlaceForAck - 1;
                                }
                                t++;
                                if(t >= windowSize)
                                {
                                    t = 0;
                                }
                            }
                        }
                    }
                    state = 0;
                }
                break;



            case 3:
                ingsoc_writeMessage(*fileDescriptor, &queue[PackToResend], sizeof(ingsoc), hostInfo);
                printf("Client - Package %d resent , SEQ nr: %d\n", PackToResend, (int) (queue[PackToResend]).SEQ);
                sent[PackToResend] = clock();
                state = 0;
                break;


            case 4:

                printf("Client - No more packages to send");
                running = 0;
                free(buffer);
                free(queue);
                free(sent);
                free(populated);
                break;

        }
    } while(running == 1);
}
void client_main(char *addres)
{
    struct sockaddr_in SERVER_NAME;
    int GSOCKET, windowSize = 0;
    fd_set GFD_SET;
    windowSize = client_connect(&GSOCKET, &GFD_SET, addres, &SERVER_NAME);
    SWSend(&GSOCKET, &GFD_SET, &SERVER_NAME, windowSize);
    printf("--Initing client_dis_connect---\n");
    client_dis_connect(&GSOCKET, GFD_SET, &SERVER_NAME);
}
