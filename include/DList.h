#ifndef DLIST_H
#define DLIST_H

#include <cstddef>
#include <cstdint>
#include <cassert>
#include <stdio.h>

struct  DList {
    DList *prev =  NULL;
    DList *next =  NULL;
};


void dlist_init(DList *node);

bool dlist_empty(DList *node);

void dlist_detach(DList *node);

void dlist_insert_before(DList *target,  DList * rookie);


#endif // DLIST_H

