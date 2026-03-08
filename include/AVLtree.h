#ifndef AVLTREE_H
#define AVLTREE_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

struct AVLNode {
    uint32_t depth = 0;
    uint32_t cnt = 0;
    AVLNode* left = nullptr;
    AVLNode* right = nullptr;
    AVLNode* parent = nullptr;
};


struct Data {
    AVLNode node;
    uint32_t val = 0;
};

struct Container {
    AVLNode * root = NULL;
};

void avl_init(AVLNode* node);

uint32_t avl_depth(AVLNode * node);

uint32_t avl_cnt(AVLNode * node);

uint32_t max(uint32_t lhs, uint32_t rhs);

AVLNode * avl_fix(AVLNode * node);

AVLNode * avl_del(AVLNode *node);

void add(Container &c,uint32_t val);

bool del(Container &c, uint32_t val);

AVLNode *avl_offset(AVLNode *node,  int64_t offset);

#endif // AVLTREE_H

