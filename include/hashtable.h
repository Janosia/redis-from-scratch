#pragma once 

#include <iostream>
#include <cstdint>
#include <cstddef>
#include <cassert>
class HNode{
    public:
        HNode *next = NULL;
        uint64_t hcode =0 ; 
};

class HTab{
    public:
        HNode **tab = NULL;
        size_t mask =0;
        size_t size=0;
};

class HMap{
    public:
        HTab newer;
        HTab older;
        size_t migrate_pos=0;
};

class Hkey{
    public:
        HNode node;
        const char *name = NULL;
        size_t len = 0;
};

// APIs
inline void h_init(HTab *htab, size_t n){
    // taking modulo using power of 2 since actual mod is CPU heavy operation
    assert(n>0 && ((n-1)&n) == 0); 
    htab->tab = (HNode **)calloc(n, sizeof(HNode *));
    htab->mask = n-1;
    htab->size = 0;
};
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode*));
void *hm_insert(HMap *hmap, HNode *node);
HNode *h_delete(HMap *hmap, HNode *key, bool(*eq) (HNode*, HNode*));
void   hm_clear(HMap *hmap);
HNode* hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode * , HNode *));
void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *arg);
inline size_t hm_size(HMap *hmap){
    return hmap->newer.size+hmap->older.size;
};
void h_insert(HTab *htab, HNode *node);