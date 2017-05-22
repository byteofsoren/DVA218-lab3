#include "ingsoc.h"

#define HOSTNAMELENGHT 50
/*If we start the program without an argument, it starts the server and if
 * we start the program with an argument e.g. "127.0.0.1" it starts the client*/
int main(int argc, char *argv[]) {
    char hostname[HOSTNAMELENGHT];
    srand(time(NULL));
    ingsoc_show_error_chance();
    if (argv[1] == NULL) {

        printf("Starting server..\n");
        Server_Main();

    } else {

        strncpy(hostname, argv[1], HOSTNAMELENGHT);
        printf("Starting a client section\nConnecting to  %s\n", hostname);
        client_main(hostname);
    }
    printf("Program ended\n\n");
    return 0;
}
