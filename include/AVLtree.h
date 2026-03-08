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




#endif // AVLTREE_H

