#include "ingsoc.h"

#define HOSTNAMELENGHT 50

int main(int argc, char *argv[])
{
    char *data = "Hejsan jag undrar hur stort MEDDELANDE man kan KORA??";
    int cksum[100];
    int a = 1, b = 2, c = 3, d = 4;

    printf("XOR test: 1 ^ 1 = %d; 1 ^ 2 = %d; 3 ^ 3 = %d; 4 ^ 3 = %d\n", a ^ a, a ^ b, c ^ c, d ^ c);


    for(int i = 0; i <= 2; i++) {

        cksum[i] = checkSum(data, strlen(data), 1);
        printf("Checksum: %d\n", cksum[i]);
    }
    char hostname[HOSTNAMELENGHT];
    printf("hello\n");
    if (argv[1] == NULL) {
        // asumes its a server
        printf("starting server\n");
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
