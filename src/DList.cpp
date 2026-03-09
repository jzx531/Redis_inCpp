#include "DList.h"

void dlist_init(DList *node){
    node->prev = node;
    node->next = node;
}

bool dlist_empty(DList *node){
    return node->next == node;
}

void dlist_detach(DList *node){
    DList *prev = node->prev;
    DList *next = node->next;
    prev->next = next;
    next->prev = prev;

    //在外部释放内存
    // delete node;
}

void dlist_insert_before(DList *target,  DList * rookie)
{
    DList *prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
}

