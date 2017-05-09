#include "doublethink.h"


doublethink_t* doublethink_alloc(){
    doublethink_t *d = (doublethink_t*) calloc(1, sizeof(doublethink_t));
    if (d != NULL) {
        d->first = d->last = NULL;
    }
    return d;
}

void doublethink_free(doublethink_t *tobefree){
    free(tobefree);
}


int doublethink_is_empty(doublethink_t *dque){
    if(dque->first != NULL)
        return 1;
    return 0;
}

struct node_s *doublethink_create_node(Data_t data){
    struct node_s *newNode = (struct node_s*) calloc(1,sizeof(struct node_s));
    if(newNode != NULL){
        newNode->next = NULL;
        newNode->prew = NULL;
        newNode->Data=data;
    }
    return newNode;
}

void doublethink_push_front(doublethink_t *dque, Data_t data){
    struct node_s *newNode = doublethink_create_node(data);
    newNode->next = dque->first;
    if (dque->last == NULL) {
        dque->first = dque->last = newNode;
    } else {
        dque->first->prew = newNode;
        dque->first = newNode;
    }
}

void doublethink_push_back(doublethink_t *dque, Data_t data){
    struct node_s *newNode = doublethink_create_node(data);
    newNode->prew = dque->last;
    if (dque->first == NULL) {
        dque->first = dque->last = newNode;
    } else {
        dque->last->next = newNode;
        dque->last = newNode;
    }
}

Data_t doublethink_pop_front(doublethink_t *dque){
    Data_t dataTemp;
    dataTemp = dque->first->Data;
    struct node_s *n = dque->first;
    if (dque->first == dque->last) {
        dque->first = dque->last = NULL;
    } else {
        dque->first = n->next;
    }
    free(n);
    return dataTemp;
}


Data_t doublethink_pop_back(doublethink_t *dque){
    Data_t dataTemp;
    dataTemp = dque->last->Data;
    struct node_s *n = dque->last;
    if (dque->first == dque->last) {
        dque->first = dque->last = NULL;
    } else {
        dque->last = n->prew;
    }
    free(n);
    return dataTemp;
}

Data_t doublethink_peak_front(doublethink_t *dque){
    return dque->first->Data;
}


Data_t doublethink_peak_back(doublethink_t *dque){
    return dque->last->Data;
}
