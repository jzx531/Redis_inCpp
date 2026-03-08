#include "AVLTree.h"
#include "Command.h"
#include <cassert>
#include <set>
#include "SortedSet.h"

// 以下是验证avl树的代码：

static void avl_verify(AVLNode *parent, AVLNode *node)  {
    if (!node)  {
        return;
    }

    assert(node->parent == parent);
    avl_verify(node, node->left);
    avl_verify(node, node->right);

    assert(node->cnt == 1 + avl_cnt(node->left) + avl_cnt(node->right));

    uint32_t l = avl_depth(node->left);
    uint32_t r = avl_depth(node->right);
    assert(l == r || l + 1 == r || l == r + 1);
    assert(node->depth == 1 + max(l, r));

    uint32_t val = container_of(node, Data, node)->val;
    if (node->left)  {
        assert(node->left->parent == node);
        assert(container_of(node->left, Data, node)->val <= val);
    }
    if (node->right)  {
        assert(node->right->parent == node);
        assert(container_of(node->right, Data, node)->val >= val);
    }
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted)  {
    if (!node)  {
        return;
    }
    extract(node->left, extracted);
    extracted.insert(container_of(node, Data, node)->val);
    extract(node->right, extracted);
}

static void container_verify(
        Container &c, const std::multiset<uint32_t> &ref)
{
    avl_verify(NULL, c.root);
    assert(avl_cnt(c.root) == ref.size());
    std::multiset<uint32_t> extracted;
    extract(c.root, extracted);
    assert(extracted == ref);
}

static void dispose(Container &c)  {
    while (c.root)  {
        AVLNode *node = c.root;
        c.root = avl_del(c.root);
        delete container_of(node, Data, node);
    }
}

static void test_insert(uint32_t sz)  {
    for (uint32_t val = 0; val < sz; ++val)  {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i)  {
            if (i == val)  {
                continue;
            }
            add(c, i);
            ref.insert(i);
        }
        container_verify(c, ref);

        add(c, val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_remove(uint32_t sz)  {
    for (uint32_t val = 0; val < sz; ++val)  {
        Container c;
        std::multiset<uint32_t> ref;
        for (uint32_t i = 0; i < sz; ++i)  {
            add(c, i);
            ref.insert(i);
        }
        container_verify(c, ref);

        assert(del(c, val));
        ref.erase(val);
        container_verify(c, ref);
        dispose(c);
    }
}


static void test_case(uint32_t sz)  {
    Container c;
    for  (uint32_t i =  0;  i <  sz;  ++i)  {
        add(c,  i);
    }

    AVLNode *min =  c.root;
    while  (min->left)  {
        min =  min->left;
    }
    for  (uint32_t i =  0;  i <  sz;  ++i)  {
        AVLNode *node =  avl_offset(min,  (int64_t)i);
        assert(container_of(node,  Data,  node)->val ==  i);

        for  (uint32_t j =  0;  j <  sz;  ++j)  {
            int64_t offset =  (int64_t)j -   (int64_t)i;
            AVLNode *n2 =  avl_offset(node,  offset);
            assert(container_of(n2,  Data,  node)->val ==  j);
        }
        assert(!avl_offset(node,  -(int64_t)i -  1));
        assert(!avl_offset(node,  sz -  i));
    }

    dispose(c);
}


int main()
{
    printf("卧药腌牌\r\n");
    printf("开始测试AVL树..\r\n");
    Container c;

    // 一些快速测试
    container_verify(c, {});
    add(c, 123);
    container_verify(c, {123});
    assert(!del(c, 124));
    assert(del(c, 123));
    container_verify(c, {});

    // 顺序插入
    std::multiset<uint32_t> ref;
    for (uint32_t i = 0; i < 1000; i += 3)  {
        add(c, i);
        ref.insert(i);
        container_verify(c, ref);
    }

    // 在不同位置进行插入/删除操作
    for (uint32_t i = 0; i < 200; ++i)  {
        test_insert(i);
        test_remove(i);
    }
    test_case(64);
    printf("测试完成,没有错误\n");
    printf("牌没有温提\r\n");
}