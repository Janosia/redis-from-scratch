#pragma once 

#include <iostream>
#include <memory>
#include <functional>

// height of avl trees : the height of 2 subtrees can have at most difference of  1 

class AVLNode{
    public:
        AVLNode * parent = NULL;
        AVLNode *left = NULL;
        AVLNode *right = NULL;
        uint32_t height = 0; // ht
        uint32_t cnt = 0;  // size of augmented subtree : size of subtree is augmented for rank queries
};
inline void avl_init(AVLNode *node){
    node->left = node->right = node->parent = NULL;
    node->height =1;
    node->cnt =1;
}

inline uint32_t avl_height (AVLNode *node ){return node ? node->height : 0;}
inline uint32_t avl_cnt(AVLNode *node){return node ? node->cnt : 0 ;}


// API
AVLNode *avl_fix(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
AVLNode *avl_offset(AVLNode *node, int64_t offset);