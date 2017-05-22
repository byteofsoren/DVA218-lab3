#include "ingsoc.h"

#define PORT 5555

size_t LatestRecSeq;
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
/* server_disconnect - This function is initialized when the server receives a FIN (disconnect request)
 * from the client*/
void server_disconnect(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {
    /* Variables:
    * toWrite, toRead - These are the structs to write and read with between client and server
    * n - Counter
    * timer - Used by select to create timeouts
    * readFdSet - Used by select and FD_ISSET to see which sockets there is input on (We only use one) */

    ingsoc toRead, toWrite;
    int n = 0;
    struct timeval timer;
    fd_set readFdSet;

    /* Starting by initializing our structs, setting all values to false and zero
     * then setting up a sequence number */

    ingsoc_init(&toRead);
    ingsoc_init(&toWrite);
    ingsoc_seqnr(&toWrite);
    toWrite.ACK = true;
    toWrite.FIN = true;
    toWrite.ACKnr = LatestRecSeq;

    do {

        /* We have already received a disconnect request from the client and will send back an ACK+FIN to
         * acknowledge that the server will also disconnect in a few moments, it will try to do so 3 times
         * before giving up and just disconnecting if no final ACK is received */

        ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
        printf("Server - FIN+ACK sent to client with SEQ: %d Answer to: %d\n", (int) toWrite.SEQ, (int) toWrite.ACKnr);

        /* Setting a timer for select to wait before calling a timeout */
        timer.tv_sec = 1;
        timer.tv_usec = 0;
        readFdSet = *activeFdSet;
        /* Looking for changes in FD */
        if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
            perror("Server - Select failure");
        /* Looks if the socket is set, then if we receive the final ACK with correct
         * SEQnr, which is the client telling the server it will disconnect,
         * the server will proceed to disconnect aswell */
        if (FD_ISSET(*fileDescriptor, &readFdSet)) {
            if (ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0) {
                if (toRead.ACK == true && toRead.ACKnr == toWrite.SEQ) {
                    printf("Server - ACK received (%d), disconnecting.\n", (int) toRead.SEQ);
                    n = 4;
                }
            }
        }
            /* Count timeouts, after 3, proceed to disconnect anyway */
        else {
            n++;
            printf("Server - timeout %d\n", n + 1);
        }
    } while (n <= 3);
}
/* Threeway - This is the server part of the threeway handshake
 * Inputs:
 * fileDescriptor - Socket handle
 * activeFdSet - List of active FDs (which is only one, port 5555)
 * hostInfo - struct for handling internet addresses */
int Threeway(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo) {

    /* Variables:
     * toWrite, toRead - These are the structs to write and read with between client and server
     * state - Used in the state-machine
     * running - An indicator for the while loop in which the state-machine is in
     * n - Counter
     * windowSize - A randomized number to represent the windowsize used later in sliding window
     * timer - Used by select to create timeouts
     * readFdSet - Used by select and FD_ISSET to see which sockets there is input on (We only use one) */

    ingsoc toWrite, toRead;
    int state = 0;
    int running = 1;
    int n = 0, windowSize = (int) ingsoc_randomNr(3, 20);
    struct timeval timer;
    fd_set readFdSet;

    do {
        switch (state) {
            /* case 0 - Idle state, waiting for a SYN from client, then agreeing on a windowSize
             * to use in sliding window*/
            case 0:
                readFdSet = *activeFdSet;
                /* Looking for changes in FD, since this is the idle state it will
                 * wait forever for an incoming connection */
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0)
                    perror("Server - Select failure");
                /* FD_ISSET looks if the socket is set */
                if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads the package from client */
                    if (ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0) {
                        /* If it receives the SYN from client it proceeds to the next state */
                        if (toRead.SYN == true) {
                            printf("Server - SYN received on %d\n", (int)toRead.SEQ);
                            /* Deciding on what windowSize to use */
                            if (toRead.length < windowSize) {
                                windowSize = toRead.length;
                            }
                            state = 1;
                        }
                    }
                }
                break;
                /* case 1 - Send SYN + ACK to client and SEQ nr, then wait for final ACK */
            case 1:
                /* ingsoc_init just initializes the struct we want to send, setting everything to false and 0,
                 * then we set our values */
                ingsoc_init(&toWrite);
                toWrite.ACK = true;
                toWrite.SYN = true;
                toWrite.length = windowSize;
                ingsoc_seqnr(&toWrite);
                toWrite.ACKnr = toRead.SEQ;

                do {
                    /* Sends the package to client */
                    ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                    printf("Server - ACK + SYN sent on %d with SEQ: %d\n",(int)toWrite.ACKnr, (int)toWrite.SEQ);
                    /* set timer to tell select for how long to look for a change before calling a timeout */
                    timer.tv_sec = 1;
                    timer.tv_usec = 0;
                    readFdSet = *activeFdSet;
                    /* Looks for changes in FD */
                    if (select(FD_SETSIZE, &readFdSet, NULL, NULL, &timer) < 0)
                        perror("Server - Select failure");

                    if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                        /* We handle the checksum in ingsoc_readMessage, and if it doesnt match up
                         * it will return -1, we then reset toRead and go back and resend
                         * the package. */
                        if (ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == -1) {
                            ingsoc_init(&toRead);
                        }
                        /* After sending SYN+ACK and receiving the final ack from client
                         * it will proceed to the next state, which is the final state */
                        if (toRead.ACK == true && toRead.ACKnr == toWrite.SEQ) {
                            printf("Server - final ACK received for %d\n", (int) toRead.SEQ);
                            state = 2;
                        }

                        /* After 3 failed attempts we will go back to the idle state */
                        else
                        {
                            printf("Server - ACK not received, attempt: %d\n", n + 1);

                            n++;
                        }
                    } else {
                        printf("Timeout\n");
                    }
                } while (state == 1 && n <= 3);

                break;

            case 2:
                /* This is the final state, once we get here the client and server is
                 * connected and the threeway handshake has been successful, from here
                 * we will proceed with the sliding window protocol. */
                LatestRecSeq = toRead.SEQ;
                printf("Server - Three-way handshake successful\n");
                running = 0;
                break;

        }
    } while (running == 1);
    return windowSize;
}
void SWRecv(int *fileDescriptor, fd_set *activeFdSet, struct sockaddr_in *hostInfo, int windowSize)
{
    size_t StartSEQ = LatestRecSeq;	//used for printing packet nr nothing more
    ingsoc toWrite, toRead;
    int state = 0;
    size_t toACK = 0;		//The place in the window of the current packet. 
    size_t running = 1;
    size_t NrInWindow = 0;		//amount of packets in the window
    size_t PlaceInWindow = 0;	//The place in the window of the oldest packet. (next in the sequence). LastRecSeq is about the same here but for the SEQ nr itsealf
    bool oldPackage = false;	//stupid variable for solving a stupid problem with the last ack from client in connect coming as a ghost in SW. Easiest way to solve it
    int i;
    fd_set readFdSet;
    int offset = 0;
    char *message = malloc(MAXMSG);
    memset(message, '\0', MAXMSG);
    int PlaceInMessage = 0;
    ingsoc *Window = malloc(windowSize * sizeof(ingsoc));

    bool *populated = malloc(windowSize * sizeof(bool));
    for (i = 0; i < windowSize; i++) {
        populated[i] = false;
    }
    ingsoc_init(&toRead);
    ingsoc_init(&toWrite);

    do {
        switch (state) {

            case 0:
				/*	case 0 do the reading and sends it of to case 1 for answering with an ack if needed.
				 *	First of there is standard select + FD_ISSET for checking message on our port 5555*/
                readFdSet = *activeFdSet;
                if (select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0)
                    perror("Server - Select failure");

                if (FD_ISSET(*fileDescriptor, &readFdSet)) {
                    /* Reads the package from client */
                    if (ingsoc_readMessage(*fileDescriptor, &toRead, hostInfo) == 0) {
						/*	When the fin message comes we move on to case 8 (jump in numbers I know) for print of message and handover to teardown*/
                        if (toRead.FIN == true) {
                            state = 8;
                        } else {
							/*	When there is no fin message firstly we're checking on old places in the window is the read message is a one in the window
							 *	Happen when the ack is lost and resend is done on the client side. This is packets with a later SEQ than LatestRecSeq*/
                            for (i = 0; i < windowSize; i++) {
                                if (toRead.SEQ == Window[i].SEQ && populated[i] == true) {
                                    state = 1;
                                }
                            }
							/*	If the message was not active in the window*/
                            if (state != 1) {
								/*	Firstly if the SEQ number is a old one a new ack is sent. Packets with Older (lower) SEQ than LatestRecSeq
								 *	oldPackage is set since there is no spot in the window for that package and the row to set the Ack trigg should not be done*/
                                if (LatestRecSeq - toRead.SEQ < 200) {
                                    state = 1;
                                    oldPackage = true;
								
								/*	Here is the main function for new packages. Will run if the packet offset from last acked fit in the empty window space.
								 *  This should be a problem if 1,3,4,5,6,7,8 arrives since the the empty space at nr 4 is 5. When the 5 arrives the free space is 4.
								 *	But there is no problem and the window is filled.  */
                                } else if (toRead.SEQ - LatestRecSeq <= windowSize - NrInWindow) {
                                    state = 1;
                                    /*  The place in the window for the packet. Put at the right spot directly*/
                                    toACK = PlaceInWindow + (toRead.SEQ - LatestRecSeq - 1);
                                    if ((int) toACK >= windowSize) {
                                        toACK -= windowSize;
                                    }
                                    /*  Put in the packet into the window and populated trigger is added*/
                                    Window[toACK] = toRead;
                                    populated[toACK] = true;
                                    /*  If the packet have the same place in the window as the one wanted. (the oldest SEQ packet)*/
                                    if (toACK == PlaceInWindow) {
                                        LatestRecSeq = toRead.SEQ;
                                    }
                                    /*  Otherwise the packet in this case will be a SEQ further into the future and offset helps noting this for later*/
                                    else {
                                        offset++;
                                    }
                                    /*  One more packet in the window is active*/
                                    NrInWindow++;
                                }
                            }
                        }
                    }
                }
                break;

            case 1:

                printf("Server - Package %ld received, SEQnr: %d\n", (toRead.SEQ - StartSEQ), (int) toRead.SEQ);
				/*	 When the packet is the next in the sequence*/
                if (toACK == PlaceInWindow && Window[PlaceInWindow].ACK == false && populated[PlaceInWindow] == true) {
					/*	Putting the data in the packet into the message buffer*/
                    for (i = 0; i < Window[PlaceInWindow].length; i++) {
                        message[PlaceInMessage] = Window[PlaceInWindow].data[i];
                        PlaceInMessage++;
                    }
					/*	Tells that the space is free to use and that the new latest packet (in the sequence) is this one. */
                    populated[PlaceInWindow] = false;
                    LatestRecSeq = Window[PlaceInWindow].SEQ;
                    NrInWindow--;
                    PlaceInWindow++;
                    if ((int) PlaceInWindow >= windowSize) {
                        PlaceInWindow = 0;
                    }
					/* If there was packages that were acked before that and is the next one in the sequence they are put in the message buffer. 
					 * Does this one at a time as long as the packet that meets the requierments then moves on*/
                    while (populated[PlaceInWindow] == true && offset > 0) {
                        populated[PlaceInWindow] = false;
                        LatestRecSeq = Window[PlaceInWindow].SEQ;
                        for (i = 0; i < Window[PlaceInWindow].length; i++) {
                            message[PlaceInMessage] = Window[PlaceInWindow].data[i];
                            PlaceInMessage++;
                        }
                        PlaceInWindow++;
                        NrInWindow--;
                        if ((int) PlaceInWindow >= windowSize) {
                            PlaceInWindow = 0;
                        }
                        offset--;
                    }

                }
				/* A safty for old package that is older than the window (they will NOT run this). They are not suppose to be placed in the window*/
                if (oldPackage == false) {
					/*	Put a trigger on the current packet so if it was not in the right order it is fixed above later when the blocking packet arrive*/
                    Window[toACK].ACK = true;
                }
                oldPackage = false;
				
				/*	Sends a responce ack and goes back to the "waiting for new packet state" (state 0)*/
                ingsoc_init(&toWrite);
                ingsoc_seqnr(&toWrite);
                toWrite.ACK = true;
                toWrite.ACKnr = toRead.SEQ;
                ingsoc_writeMessage(*fileDescriptor, &toWrite, sizeof(toWrite), hostInfo);
                printf("Sending ACK on %d with SEQ: %d\n", (int) toWrite.ACKnr, (int)toWrite.SEQ);
                state = 0;
                break;
            case 8:
				/*	The ender of the server. It finnish the message with a \0, print the total mesage sent and free all dynamic variables. Then disconnect will happen*/
                message[PlaceInMessage] = '\0';
                /* Writes out message with green text \e[032m */
                printf("FIN received with SEQ: %d\n", (int)toRead.SEQ);
                printf("--Message was--\n[\e[032m%s \e[0m]\n", message);
                running = 0;
                free(message);
                free(populated);
                free(Window);
                LatestRecSeq = toRead.SEQ;
                break;
        }

    } while (running == 1);
}
void Server_Main(int arg){
    printf("arg=%d\n",arg);
    int fileDescriptor;
    struct sockaddr_in  hostInfo;
    int windowSize = 0;
    // int nOfBytes = 0;
    //char buffer[MAXMSG];
    //fd_set readFdSet;
    fd_set activeFdSet; /* Used by select */
    fileDescriptor = make_Socket4(PORT);
    FD_ZERO(&activeFdSet);
    FD_SET(fileDescriptor,&activeFdSet);
/* Create a socket and set it up to accept connections */

    /* Initialize the set of active sockets */

    //server_disconnect(&sock, &activeFdSet, &hostInfo);

    printf("\n[waiting for connections...]\n");

    windowSize = Threeway(&fileDescriptor, &activeFdSet, &hostInfo);
    printf("Window size is %d\n", windowSize);
    SWRecv(&fileDescriptor, &activeFdSet, &hostInfo, windowSize);
    server_disconnect(&fileDescriptor, &activeFdSet, &hostInfo);

}

