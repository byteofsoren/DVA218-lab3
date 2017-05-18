#include "ingsoc.h"

#define PORT 5555
#define hostNameLength 50
#define messageLength  256

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
    ingsoc rAck;
    ingsoc sACK;
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
                    printf("Sending SYN to server with %d\n", (int) sSyn.SEQ);
                    timer.tv_sec = 1;
                    timer.tv_usec = 0;
                    GFD_SET = *ActiveFdSet;
                    int t = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                    if (t == -1) {
                        perror("select");
                    }
                    if (FD_ISSET(*GSOCKET, &GFD_SET)) {


                        if(ingsoc_readMessage(*GSOCKET, &rAck, SERVER_NAME) == 0)
                        {
                            if (rAck.ACK == true && rAck.SYN == true && rAck.ACKnr == sSyn.SEQ) {
                                printf("ACK + SYN received for %d with SEQ %d\n", (int)rAck.ACKnr, (int)rAck.SEQ);
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
                ingsoc_init(&sACK);
                ingsoc_seqnr(&sACK);
                sACK.ACK = true;
                sACK.ACKnr = ACK_NR;

                do {
                    printf("Sending ACK on %d with SEQ: %d\n", (int) rAck.SEQ, (int) sACK.SEQ);
                    ingsoc_writeMessage(*GSOCKET, &sACK, sizeof(sACK), SERVER_NAME);
                    struct timeval timer;
                    timer.tv_sec = 5;
                    timer.tv_usec = 0;
                    printf("Reading socket in final state\n");
                    GFD_SET = *ActiveFdSet;
                    int stemp = select(FD_SETSIZE, &GFD_SET, NULL, NULL, &timer);
                    if (stemp == -1) {
                        printf("Problem with select in sate 1");
                    }

                    if (FD_ISSET(*GSOCKET, &GFD_SET)) {
                        ingsoc rACK;
                        ingsoc_readMessage(*GSOCKET, &rACK, SERVER_NAME);
                        if (rACK.ACK == true && rACK.SYN == true && rACK.ACKnr == sSyn.SEQ) {
                            printf("Received ACK + SYN in final state\n");
                        }
                    } else {
                        // time out exits the loop
                        running = 0;    // Stops the program
                        printf("No duplicate packets...continuing to sliding Windows\n");
                    }
                }while(running == 1);
                break;
            }
        }
    }
    printf("--- END ---\n\tEnded 3 way handshake function\n");
    return windowSize;
}

int client_dis_connect(int *GSOCKET, fd_set *ActiveFdSet, struct sockaddr_in *SERVER_NAME)
{
    /* This is the disconnect function */
    printf("Starting client client_dis_connect\n");
    int counter = 3;
    fd_set GFD_SET = *ActiveFdSet;
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
        timer.tv_sec = 5;
        timer.tv_usec = 0;
        printf("Waiting for fin + ack\n");
        GFD_SET = *ActiveFdSet;
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
                    printf("ACK sent on %d with SEQ: %d\n", (int)rAck.SEQ, (int)sFin.SEQ);
                    return 0;
                }
                else
                {
                    printf("No FIN + ACK revived\n");
                }
            }
        }
        else
        {
            printf("Timeout\n");
        }
        counter--;
    }
    // wait for FIN + ACK
    // Send ACK + FIN
    // close
    return 0;
}
void SWSend(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo, int windowSize)
{
    ingsoc toWrite, toRead;//window[windowSize];
    ingsoc *queue = malloc(windowSize * sizeof(ingsoc));
    clock_t *sent = malloc(windowSize * sizeof(clock_t));
    int state = 0;
    int i,t = 0,  PackToResend;
    int running = 1;
    int PlaceInWindow = 0;      //where in the windows we are
    int PlaceForAck = 0;
    int length = 0;
    int NrInWindow = 0;     //how many packages there is in the window at a given time
    int PlaceInMessage = 0;     //where in the string to be sent we are
    size_t StartSEQ = 0;
    char *buffer = malloc(MAXMSG);
    memset(buffer, '\0', MAXMSG);
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

    /*This do is for the running of Client Sliding Window.*/
    do {
        /*  Goes through all window places in sliding window to check if some packet that have been sent
         *  needs a timout. So a populated place that has not received an ACK*/
        for (i = 0; i < windowSize; i++)
        {
            if((clock() - sent[i]) > 10000 && populated[i] == true && queue[i].ACK == false)
            {
                state = 3;
                PackToResend = i;
                i = windowSize;
            }
        }

        /*  Every lap the select is run to check for packages that have returned (on our FD, fileDescriptor)
         *  If so state will be switched to 2 to read the incoming as long as we are not in a sending state (1 & 3) and there the incoming will be managed */
        timer.tv_usec = 1;
        timer.tv_sec = 0;
        readFdSet = *activeFdSet;
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
            perror("Client - Select failure");
        
        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            if (state != 1 && state != 3) {
                state = 2;
            }
        }


        switch (state) {
            /* Case 0 have everything to do with with packet packaging and deciding when new package is to be sent */
            case 0:
                /*  Firstly when there is space in the window a new package will be handled */
                if(NrInWindow < windowSize && populated[PlaceInWindow] == false)
                {
                    state = 1;

                    if(buffer[PlaceInMessage] == '\0' && NrInWindow == 0)
                    {
                        state = 4;
                    }
                    else
                    {
                        /*  Filling the new package with its message depending on the situation.
                         *  In case when the total message left is smaller than what the maximum we send we just send the rest.
                         *  Otherwise we randomise (for now between 1 and 3) how much to be sent in this package*/
                        if (length - PlaceInMessage < 3)
                        {
                            toWrite.length = (length - PlaceInMessage);
                        } else
                        {
                            toWrite.length = ingsoc_randomNr(1, 3);
                        }
                        for (i = 0; i < toWrite.length; i++)
                        {
                            toWrite.data[i] = buffer[PlaceInMessage];
                            PlaceInMessage++;
                        }

                        /*  Now a \0 is added at the end of the message just to make it look nice.
                         *  Since data in ingsoc is 256 bytes there will a lot of scrap data sending small messages.
                         *  But \0 will make one destinction when debugging or printing a specific message
                         *  Then a SEQ number is added, it is placed in the queue, specified as populated place in window.*/
                        toWrite.data[toWrite.length] = '\0';
                        ingsoc_seqnr(&toWrite);
                        queue[PlaceInWindow] = toWrite;
                        //sent[PlaceInWindow] = clock();
                        populated[PlaceInWindow] = true;
                        NrInWindow++;

                        /* This little sequence is just a variable used for printing package number to the user */
                        if(StartSEQ == 0)
                        {
                            StartSEQ = toWrite.SEQ;
                        }
                    }
                }
                /*  In case when there is no space in the window there is no other thing to do than wait for a package ACK to be returned. 
				 *  But since we also want to timeout older packages this is not infinite to it goes back to the first section to check on sent packages.*/
                else {
                    timer.tv_usec = 1000;
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
				/*	The last messsage in the queue is sent a way which is the latest created message
				 *	Then a nice message is sent. The first variable in that is the package number where StartSEQ is the first sent package*/
                ingsoc_writeMessage(*fileDescriptor, &queue[PlaceInWindow], sizeof(ingsoc), hostInfo);
                printf("Client - Package %ld sent, SEQ nr: %d\n", (queue[PlaceInWindow].SEQ - StartSEQ), (int) (queue[PlaceInWindow]).SEQ);
				
				/*	The sent package is timestimestamped and the window is moved one more spot (or to the begining if the end of the windows size is reached)
				 *	Then its is time to go back to the 0 state to create a new package*/
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
				/* check if there is a change in our FD or there has been a tieout*/ 
                if(FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads package from client if our FD has changed */
                    if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0)
                    {
						/*	The message handled if it contains an ack and a corresponding ACKnr that is the same as the SEQ of the sent message. 
						 * 	The sent message we are talking about is the one with the oldest SEQ since that would be the one we are expecting to come as soon as possible*/
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
                printf("Client - Package %ld resent , SEQ nr: %d\n", (queue[PlaceInWindow].SEQ - StartSEQ), (int) (queue[PackToResend]).SEQ);
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
    client_dis_connect(&GSOCKET, &GFD_SET, &SERVER_NAME);
}
