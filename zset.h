#pragma once 
#include "avl.h"
#include "hashtable.h"

class Zset{
    public:
        AVLNode *root = NULL;
        HMap hmap;
};

class ZNode{
    public: 
        AVLNode tree;
        HNode hmap;
        double score =0;
        size_t len =0;
        char name[0]; // reduce memory allocations
};


// POINTER query API 
bool zset_insert(Zset *zset, const char *name, size_t len, double score);
ZNode *zset_lookup(Zset *zset, const char *name, size_t len);
void zset_delete(Zset *zset, ZNode *node);
void zset_clear(Zset *zset);
static void zset_update(Zset *zset, ZNode *node, double score);

// RANGE QUERY API
ZNode *zset_seekge(Zset *zset, double score, const char *name, size_t len);
ZNode *znode_offset(ZNode *node , int64_t offset);

