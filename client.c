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
/* client_connect is the client part of the threeway handshake */
int client_connect(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {

    /* Variables:
     * state - Used by switch (state-machine)
     * readFdSet - Used by select and FD_ISSET to see which sockets there is input on (We only use one)
     * running - Used in the loop around the switch-state
     * counter - Counter
     * ACK_NR - Used to temporary save the acknr in some cases
     * windowSize - A randomized number to represent the window size used later in sliding window
     * timer - Used by select to create timeouts
     * sSyn, rAck, sACK - Structs to read and write with between server/client */

    short state = 0;
    fd_set readFdSet;
    bool running = 1;
    int counter = 5;
    size_t ACK_NR = 0;
    int windowSize = (int) ingsoc_randomNr(3, 20);
    struct timeval timer;
    ingsoc sSyn;
    ingsoc rAck;
    ingsoc sACK;

    while (running) {
        switch (state) {
            /* State 0 -  */
            case 0:
                /* Starting the 3 way handshake by initializing the struct to send, setting all
                 * values to false and zero then adding a SEQ nr */
                ingsoc_init(&sSyn);
                sSyn.SYN = true;
                sSyn.length = windowSize;
                ingsoc_seqnr(&sSyn);

                while (counter > 0 && state == 0) {

                    ingsoc_writeMessage(*fileDescriptor, &sSyn, sizeof(sSyn), hostInfo);
                    printf("Client - SYN sent with SEQ nr: %d\n", (int) sSyn.SEQ);
                    timer.tv_sec = 1;
                    timer.tv_usec = 0;
                    readFdSet = *activeFdSet;
                    int t = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer);
                    if (t == -1)
                        perror("Client - Select failed");

                    if (FD_ISSET(*fileDescriptor, &readFdSet)) {

                        if (ingsoc_readMessage(*fileDescriptor, &rAck, hostInfo) == 0) {
                            if (rAck.ACK == true && rAck.SYN == true && rAck.ACKnr == sSyn.SEQ) {
                                printf("Client - ACK+SYN received for %d with SEQ nr: %d\n", (int) rAck.ACKnr, (int) rAck.SEQ);
                                ACK_NR = rAck.SEQ;
                                windowSize = rAck.length;
                                state = 1;
                            } else {
                                printf("Client - ACK + SYN not received\n");
                            }
                        }
                    } else {
                        printf("\n------------------\nTime out counter is now \e[38;5;162m%d\e[0m\n", counter);
                        counter--;
                        state = 0;
                        if (counter == 0) exit(EXIT_FAILURE);
                    }
                }
                break;
            case 1: {
                ingsoc_init(&sACK);
                ingsoc_seqnr(&sACK);
                sACK.ACK = true;
                sACK.ACKnr = ACK_NR;

                do {
                    printf("Client - ACK sent on %d with SEQ nr: %d\n", (int) rAck.SEQ, (int) sACK.SEQ);
                    ingsoc_writeMessage(*fileDescriptor, &sACK, sizeof(sACK), hostInfo);
                    struct timeval timer;
                    timer.tv_sec = 5;
                    timer.tv_usec = 0;
                    printf("Client - Reading socket in final state\n");
                    readFdSet = *activeFdSet;
                    int stemp = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer);
                    if (stemp == -1) {
                        printf("Client - Select failed\n");
                    }

                    if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                        ingsoc rACK;
                        ingsoc_readMessage(*fileDescriptor, &rACK, hostInfo);
                        if (rACK.ACK == true && rACK.SYN == true && rACK.ACKnr == sSyn.SEQ) {
                            printf("Client - Received ACK + SYN in final state\n");
                        }
                    } else {
                        /* When running goes to zero, it exists the loop and stops the function */
                        running = 0;
                        printf("Client - Threeway handshake successful\n");
                    }
                } while (running == 1);
                break;
            }
        }
    }
    printf("--- END ---\n\tEnded 3 way handshake function\n");
    return windowSize;
}
/* This is the client disconnect function, initialized after sliding window is done sending */
int client_dis_connect(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {

    /* Variables:
     * counter - Counter
     * sFin, rAck - Structs to read and write with
     * readFdSet - Used by select and FD_ISSET to see which sockets there is input on (We only use one)
     * timer - Used by select to call timeouts */

    int counter = 3;
    fd_set readFdSet = *activeFdSet;
    ingsoc sFin;
    ingsoc rAck;
    struct timeval timer;

    /* Initializing our structs, setting everything to false and zero, then adding a SEQ nr */
    ingsoc_init(&sFin);
    ingsoc_seqnr(&sFin);
    sFin.FIN = true;
    //usleep(40);

    printf("Client - Disconnecting\n");

    /* The client initializes the disconnect by sending a FIN (disconnect request) to the server,
     * it then waits to receive a FIN+ACK and then sending the final ACK to the server before disconnecting
     * It tries to do this 3 times before disconnecting anyway */
    while (counter >= 0) {

        ingsoc_writeMessage(*fileDescriptor, &sFin, sizeof(sFin), hostInfo);
        printf("Client - FIN sent with SEQ nr: %d\n", (int) sFin.SEQ);

        /* select is looking for changes in FD for 5 sec before calling timeout
         * With each timeout it subtracts one from the counter down to zero */
        timer.tv_sec = 5;
        timer.tv_usec = 0;
        readFdSet = *activeFdSet;
        int stemp = select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer);
        if (stemp == -1)
            perror("Client - Select failed\n");
        /* Looks if the socket is set */
        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            /* Reads message for server. */
            if (ingsoc_readMessage(*fileDescriptor, &rAck, hostInfo) == 0) {
                /* When ACK+FIN is received from server, client answer with the final ACK
                 * before disconnecting */
                if (rAck.ACK == true && rAck.FIN == true && rAck.ACKnr == sFin.SEQ) {
                    printf("Client - FIN+ACK received on SEQ nr: %d\n", (int)rAck.ACKnr);

                    ingsoc_init(&sFin);
                    ingsoc_seqnr(&sFin);
                    sFin.ACK = true;
                    sFin.ACKnr = rAck.SEQ;
                    /* Sending the final ACK to server, no guarantees this will reach the server
                     * but we disconnect anyway */
                    ingsoc_writeMessage(*fileDescriptor, &sFin, sizeof(sFin), hostInfo);
                    printf("Client - ACK sent on %d with SEQ nr: %d\n", (int) rAck.SEQ, (int) sFin.SEQ);
                    return 0;

                } else {
                    printf("Client - No FIN + ACK revived\n");
                }
            }
        } else {
            printf("Client - Timeout %d\n", 4-counter);
        }
        counter--;
    }
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
    int PlaceInWindow = 0;      //where in the windows we are sending 
    int PlaceForAck = 0;		//Very similar to PlaceInWindow but based on the oldest sent position (that have not been ACKed) 
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
        /*  Goes through all window places in sliding window to check if some packet that have been sent needs a timout. 
		 *	So a populated place that has not received an ACK in a long while will be sent to state 3 for a resend
		 * 	The if statement around is a protection when senting a new package because the timestamp is set in state 1 and this could trigger on that package otherwise*/
        if(state != 1)
        {
            for (i = 0; i < windowSize; i++) {
                if ((clock() - sent[i]) > 10000 && populated[i] == true && queue[i].ACK == false) {
                    state = 3;
                    PackToResend = i;
                    i = windowSize;
                }
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
					/*	When the PlaceInMessage is at the end the last character will be a \0 and a closing of Sliding window is done at state 4*/
                    if(buffer[PlaceInMessage] == '\0' && NrInWindow == 0)
                    {
                        state = 4;
                    }
                    else
                    {
                        /*  Filling the new package with its message depending on the situation.
                         *  In case when the total message left is smaller than what the maximum we send we just send the rest.
                         *  Otherwise we randomise (for now between 1 and 3) how much to be sent in this package*/
                        if (length - PlaceInMessage < (MAX_DATA - 1))
                        {
                            toWrite.length = (length - PlaceInMessage);
                        } else
                        {
                            toWrite.length = ingsoc_randomNr(1, (MAX_DATA - 1));
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
                        populated[PlaceInWindow] = true;
                        NrInWindow++;

                        /* This little sequence is just a variable used for printing package number to the user */
                        if(StartSEQ == 0)
                        {
                            StartSEQ = (toWrite.SEQ - 1);
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
                timer.tv_usec = 0;
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                    perror("Client - Select failure");
				/* check if there is a change in our FD or there has been a tieout*/ 
                if(FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads package from client if our FD has changed */
                    if(ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0)
                    {
						/*	The message handled if it contains an ack and a corresponding ACKnr that is the same as the SEQ of the sent message. 
						 * 	The sent message we're talking about is the one with the oldest SEQ (that has not been ACKed)*/
                        if(toRead.ACK == true && toRead.ACKnr == queue[PlaceForAck].SEQ)
                        {
							/*	removes the package from the populated list and reduces the total packages nr in the window and moves the next expected one step*/
                            NrInWindow--;
                            populated[PlaceForAck] = false;
                            PlaceForAck++;
                            if(PlaceForAck >= windowSize)
                            {
                                PlaceForAck = 0;
                            }
							              printf("\e[032mACK recieved\e[0m: \e[034m%d\e[0m\n", (int) toRead.ACKnr);
                          
                            /* 	This is one step later in the window and check if newer packages has been receaved (a populated that is acked)
							               *	When the while is done the PlaceForAck should be at the spot for the oldest sent package that have not been acked.*/
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
						/*	The above was for when the read package was the next expecting one (ack for the oldest package sent
						 *	Here packages that are further into the window are marked (when ack is receaved on them) so no resends is done on them
						 *	The last part of the above if de-populates this ones when the window catch up.*/
                        else
                        {
							/*	From the spot one after the expected ack tor receive this goes through one lap until it is back at the expected ack (PlaceForAck)
							 *	When the read package is found in the sent list (queue) that is marked by the ACK flag since that flag is not used by client in SW
							 *	If that ack does not have any package in our window that it is ACKing it is just dropped. The program is then sent back to state 0*/
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
                                    printf("\e[032mACK recieved\e[0m: \e[034m%d\e[0m\n", (int) toRead.ACKnr);
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
				/*	Resends the package and updates the clock. Comes here by the first if sequence*/
                ingsoc_writeMessage(*fileDescriptor, &queue[PackToResend], sizeof(ingsoc), hostInfo);
                printf("Client - Package %ld resent , SEQ nr: %d\n", (queue[PackToResend].SEQ - StartSEQ), (int) (queue[PackToResend]).SEQ);
                sent[PackToResend] = clock();
                state = 0;
                break;


            case 4:
				/*	The end case. Free all dynamic variables and closes the sliding window part*/
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
void client_main(char *address)
{
    struct sockaddr_in hostInfo;
    int fileDescriptor, windowSize = 0;
    fd_set activeFdSet;

    fileDescriptor = socket(PF_INET, SOCK_DGRAM, 0);
    if (fileDescriptor < 0) {
        perror("Could not create a socket\n");
        exit(EXIT_FAILURE);
    }
    client_init_socket_addres(&hostInfo, address, PORT);
    FD_ZERO(&activeFdSet);
    FD_SET(fileDescriptor, &activeFdSet);

    windowSize = client_connect(&fileDescriptor, &activeFdSet, &hostInfo);
    SWSend(&fileDescriptor, &activeFdSet, &hostInfo, windowSize);
    client_dis_connect(&fileDescriptor, &activeFdSet, &hostInfo);
}
