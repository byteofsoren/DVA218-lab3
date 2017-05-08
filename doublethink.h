#ifndef DOUBLETHINK
#define DOUBLETHINK
#include <stdlib.h>
#include <stdio.h>
/*
 * 2 + 2 = 5
 * In the end the Party would announce that two and two made five, and you
 * would have to believe it.
 *
 * Doublethink in this case tho is a dual linked list
 */

typedef int Data_t;     // <-- Change this one to the data you want to send

struct node_s {
    struct node_s *next;
    struct node_s *prew;
    Data_t Data;
};

typedef struct doublethink_s {
    struct node_s *first;
    struct node_s *last;
}doublethink_t;

doublethink_t* doublethink_alloc();
void doublethink_free(doublethink_t *tobefree);
int doublethink_is_empty(doublethink_t *dque);

/* push things on the stack from ether front or back */
void doublethink_push_front(doublethink_t *dque, Data_t data);
void doublethink_push_back(doublethink_t *dque, Data_t data);

/* removes a item form the list and returns it */
Data_t doublethink_pop_front(doublethink_t *dque);
Data_t doublethink_pop_back(doublethink_t *dque);

/* Return the front back value of the que*/
Data_t doublethink_peak_front(doublethink_t *dque);
Data_t doublethink_peak_back(doublethink_t *dque);

#endif //--End h file
