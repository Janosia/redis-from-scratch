// C++ imports
#include <iostream>
#include <cstring>
#include <vector>
#include <cassert>

// Project Imports
#include "zset.h"
#include "common.h"


/*@brief  ZNode constructor. Malloc is used because cpp lacks support for flexible arrays*/
ZNode *znode_new(const char *name, size_t len, double score){
    ZNode *node = (ZNode *)malloc(sizeof(ZNode)+len);
    avl_init(&node->tree);
    node->hmap.next = NULL;
    node->hmap.hcode = str_hash((uint8_t *)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);
    return node;
}

/*@brief   deallocation of memory, this is done to prevent leaking*/
void znode_del(ZNode *node){
    free(node);
}
size_t min(size_t lhs, size_t rhs){
    return lhs < rhs ? lhs : rhs;
}

bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, hmap);
    Hkey *hkey = container_of(key, Hkey, node);
    if (znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

ZNode *zset_lookup(Zset *zset, const char *name, size_t len){
    if(!zset->root) return NULL;
    Hkey key;
    key.node.hcode = str_hash((uint8_t *)name, len);
    key.name = name;
    key.len = len;
    HNode *found = hm_lookup(&zset->hmap , &key.node, &hcmp);
    return found ? container_of(found, ZNode, hmap): NULL;
}

bool zless(AVLNode *lhs, double score, const char *name, size_t len) {
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) {
        return zl->score < score;
    }
    int rv = memcmp(zl->name, zl->name, min(zl->len, len));
    return (rv != 0) ? (rv < 0) : (zl->len < len);
}

bool zless(AVLNode *lhs, AVLNode *rhs){
    ZNode *zr = container_of(rhs, ZNode, tree);
    return zless(lhs, zr->score, zr->name, zr->len);
}

void insert(Zset *zset, ZNode *node){
    AVLNode *parent = NULL;
    AVLNode **from = &zset->root;
    while(*from){
        parent = *from;
        from = zless(&node->tree, parent) ? &parent->left : &parent->right;
    }
    *from = &node->tree;
    node->tree.parent =  parent;
    zset->root = avl_fix(&node->tree);
}

bool zset_insert(Zset *zset, const char *name, size_t len, double score){
    if(ZNode *node = zset_lookup(zset, name, len)){
        zset_update(zset, node, score);
        return false;
    }
    ZNode *node = znode_new(name, len , score);
    hm_insert(&zset->hmap, &node->hmap);
    insert(zset, node);
    return true;
}

void zset_update(Zset *zset, ZNode *node, double score){
    zset->root = avl_del(&node->tree);
    avl_init(&node->tree);
    node->score = score;
    insert(zset, node);
}

void zset_delete(Zset *zset, ZNode *node){
    Hkey key;
    key.node.hcode = node->hmap.hcode;
    key.name = node->name;
    key.len = node->len;
    HNode *found = hm_delete(&zset->hmap, &key.node, &hcmp);
    assert(found);
    zset->root = avl_del(&node->tree);
    znode_del(node);
}

ZNode *zset_seekge(Zset *zset, double score , const char *name, size_t len){
    AVLNode *found = NULL;
    for(AVLNode *node = zset->root; node;){
        if(zless(node, score, name, len)){
            node = node->right;
        }else {
            found = node;
            node = node->left;
        }
    }
    return found ? container_of(found , ZNode , tree) : NULL;
}

ZNode *znode_offset(ZNode *node, int64_t offset){
    AVLNode *tnode = node ? avl_offset(&node->tree, offset) : NULL;
    return tnode ? container_of(tnode, ZNode, tree) : NULL;
}

/*@brief delete entire AVL tree*/
void tree_dispose(AVLNode *node) {
    if (!node) {
        return;
    }
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}
/*@brief  Destroying Zset*/
void zset_clear(Zset *zset) {
    hm_clear(&zset->hmap);
    tree_dispose(zset->root);
    zset->root = NULL;
}

