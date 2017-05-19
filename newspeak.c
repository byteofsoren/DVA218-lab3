#include "newspeak.h"
#include "ingsoc.h"

void newspeak(ingsoc *s){
    if(NO_LIES == 1){
        char t[] = "\e[032mtrue\e[0m";
        char f[] = "\e[031mfalse\e[0m";
        printf("ACK=%s, FIN=%s, RES=%s, SYN=%s\n",
                s->ACK?t:f,
                s->FIN?t:f,
                s->RES?t:f,
                s->SYN?t:f);
        printf(" ACKnr=\e[036m%ld\e[0m\n SEQ=\e[036m%ld\e[0m\n clientID=\e[036m%ld\e[0m\n", s->ACKnr, s->SEQ, s->clientID);
        printf("cksum=\e[033m%d\e[0m, length=\e[033m%d\e[0m\n", s->cksum, s->length);
        //printf("Data='%s'\n\n",s->data);
        printf("--First 32 byte of Data--\n");
        for (int i = 0; i < 32; ++i) {
            printf("%#.8X, ", s->data[i]);
            if (i % 8 == 7) printf("\n");
        }
        printf("\n--End--\n");
    }
}


void buffer_print(char *buffer, size_t bytes){
    printf("--Buffer_print--\n");
    for ( size_t    i = 0;  i < bytes ; ++ i) {
             printf("%#.8X, ", buffer[i]);
            if (i % 8 == 7) printf("\n");
    }
    printf("\n-- END --\n");
}
