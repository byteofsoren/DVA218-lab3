#include "ingsoc.h"

#define HOSTNAMELENGHT 50

int main(int argc, char *argv[])
{
    char hostname[HOSTNAMELENGHT];
    srand(time(NULL));
    printf("Hello World!\n");

    if (argv[1] == NULL) {
        // assumes its a server
        printf("Starting server..\n");
        Server_Main(0);

        //--> function to server here
    }else{
        //Assumes its a client
        strncpy(hostname, argv[1], HOSTNAMELENGHT);
        printf("Starting a client section\nConnecting to  %s\n", hostname);
        //--> function to client
        client_main(hostname);
    }
    printf("Program ended\n\n");
    return 0;
}
