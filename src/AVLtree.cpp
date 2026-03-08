#include "AVLtree.h"

static void avl_init(AVLNode* node) {
    node->left = nullptr;
    node->right = nullptr;
    node->parent = nullptr;
    node->depth = 1;
    node->cnt = 1;
}

/*----辅助函数----*/
