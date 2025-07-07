#include <iostream>

class Dlist{
    public:
        Dlist *prev = NULL;
        Dlist *next = NULL;
};

inline void dlist_inint(Dlist *node){
    node->prev = node->next = node;
}

inline bool dlist_empty(Dlist *node){
    return node->next == node;
}

inline void dlist_detach(Dlist *node){
    Dlist *prev = node->prev, *next = node->next;
    prev->next = next;
    next->prev = prev; 
}

/*
@brief : insertion in doubly linked list
@param : target : node before which the new node will be added
         rookie : node which will be added 
*/
inline void dlist_insert(Dlist *target, Dlist *rookie){
    Dlist * prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
}