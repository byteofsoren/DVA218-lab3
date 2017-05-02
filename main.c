#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

#define HOSTNAMELENGHT 50



int main(int argc, char *argv[])
{
    char *data = "Hejsan";
    int cksum;

    cksum = checkSum(data, strlen(data));
    printf("Checksum: %d\n", cksum);

    char hostname[HOSTNAMELENGHT];
    printf("hello\n");
    if (argv[1] == NULL) {
        // asumes its a server
        printf("starting server\n");
        Socket_Main(0);
        //--> function to server here
    }else{
        //Assumes its a client
        strncpy(hostname, argv[1], HOSTNAMELENGHT);
        printf("Starting a client section\nConnecting to  %s\n", hostname);
        //--> function to client
    }
    printf("Program ended\n\n");
    return 0;
}
