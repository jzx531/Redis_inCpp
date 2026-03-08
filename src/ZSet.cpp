#include "ZSet.h"

#include <cassert>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// #ifndef min
// #define min(a, b) (((a) < (b)) ? (a) : (b))
// #endif
static size_t min(size_t lhs, size_t rhs) {
    return lhs < rhs ? lhs : rhs;
}

static ZNode *znode_new(const char *name, size_t len, double score) {
    ZNode *node = (ZNode *)malloc(sizeof(ZNode) + len);
    assert(node);  
    avl_init(&node->tree);
    node->hmap.next = NULL;
    node->hmap.hcode = str_hash((uint8_t *)name, len);
    node->score = score;
    node->len = len;
    memcpy(&node->name[0], name, len);
    return node;
}

/*
有序集合由(分数,名称)对组成的有序列表，它支持通过排序键或名称进行查询和更新
由AVL树和哈希表组合而成，每个节点对同时属于这两种结构
名称字符串被嵌入在节点对的末尾
*/
//用于比较两个节点对的辅助函数
static bool zless(
    AVLNode *lhs, double score, const char *name, size_t len)
{
    ZNode *zl = container_of(lhs, ZNode, tree);
    if (zl->score != score) {
        return zl->score < score;
    }
    int rv = memcmp(zl->name, name, min(zl->len, len));
    if (rv != 0) {
        return rv < 0;
    }
    return zl->len < len;
}

static bool zless(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree);
    return zless(lhs, zr->score, zr->name, zr->len);
}

// 插入到AVL树中
void tree_add(ZSet *zset,  ZNode *node)
{
    AVLNode *parent = NULL;         // insert under this node
    AVLNode **from = &zset->tree;   // the incoming pointer to the next node
    while (*from) {                 // tree search
        parent = *from;
        from = zless(&node->tree, parent) ? &parent->left : &parent->right;
    }
    *from = &node->tree;            // attach the new node
    node->tree.parent = parent;
    zset->tree = avl_fix(&node->tree);
}

//更新现有节点的分数(AVL树重新插入)
void zset_update(ZSet *zset,  ZNode *node,  double score)
{
    if(node->score == score) return;
    zset->tree = avl_del(&node->tree);
    node->score = score;
    avl_init(&node->tree);
    tree_add(zset,node);
}

// HKey结构用于哈希查找
struct HKey {
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

static bool hcmp(HNode *node, HNode *key) {
    ZNode *znode = container_of(node, ZNode, hmap);
    HKey *hkey = container_of(key, HKey, node);
    if (znode->len != hkey->len) {
        return false;
    }
    return 0 == memcmp(znode->name, hkey->name, znode->len);
}

//按名称查找
ZNode *zset_lookup(ZSet *zset,  const char *name,  size_t len)
{
    // printf("zset_lookup: zset=%p, name=%s, len=%lu\n",zset,name,len);
    if(!zset->tree){
        printf("zset_lookup: zset->tree is NULL\n");
        return NULL;
    }
    // printf("zset_lookup: zset->tree=%p\n",zset->tree);
    HKey key;
    key.node.hcode = str_hash((uint8_t*)name,len);
    key.name = name;
    key.len = len;
    //哈希查找
    // printf("zset_lookup: key.node.hcode=%lu\n",key.node.hcode);
    HNode *hnode = hm_lookup(&zset->hmap,&key.node,&hcmp);
    return hnode? container_of(hnode,ZNode,hmap) : NULL;
}


//添加一个新的(分数,名称)元组,或者更新现有元组的分数
bool zset_add(ZSet *zset,const char *name,size_t len,double score)
{
    // printf("zset_add: zset=%p, name=%s, len=%lu, score=%f\n",zset,name,len,score);
    ZNode *node = zset_lookup(zset,name,len);
    // printf("zset_add: node=%p\n",node);
    if(node){
        zset_update(zset,node,score);
        return false;
    }
    else{
        node = znode_new(name,len,score);
        hm_insert(&zset->hmap,&node->hmap);
        tree_add(zset,node);
        return true;
    }
}

// find the first (score, name) tuple that is >= key.
ZNode *zset_seekge(ZSet *zset, double score, const char *name, size_t len) {
    AVLNode *found = NULL;
    for (AVLNode *node = zset->tree; node; ) {
        if (zless(node, score, name, len)) {
            node = node->right; // node < key
        } else {
            found = node;       // candidate
            node = node->left;
        }
    }
    return found ? container_of(found, ZNode, tree) : NULL;
}

// offset into the succeeding or preceding node.
ZNode *znode_offset(ZNode *node, int64_t offset) {
    AVLNode *tnode = node ? avl_offset(&node->tree, offset) : NULL;
    return tnode ? container_of(tnode, ZNode, tree) : NULL;
}  

// 查找大于或等于给定参数的（分数，名称）元组
ZNode *zset_query(
    ZSet *zset,  double score,  const char *name,  size_t len,  int64_t offset)
{
    ZNode *found = zset_seekge(zset, score, name, len);
    return znode_offset(found, offset);

}



static void znode_del(ZNode *node) {
    free(node);
}
//删除一个节点
void zset_delete(ZSet *zset, ZNode *node) {
    // remove from the hashtable
    HKey key;
    key.node.hcode = node->hmap.hcode;
    key.name = node->name;
    key.len = node->len;
    HNode *found = hm_pop(&zset->hmap, &key.node, &hcmp);
    assert(found);
    // remove from the tree
    zset->tree = avl_del(&node->tree);
    // deallocate the node
    znode_del(node);
}

static void tree_dispose(AVLNode *node) {
    if (!node) {
        return;
    }
    tree_dispose(node->left);
    tree_dispose(node->right);
    znode_del(container_of(node, ZNode, tree));
}

//销毁zset
void zset_clear(ZSet *zset)
{
    hm_clear(&zset->hmap);
    tree_dispose(zset->tree);
    zset->tree = NULL;
}








