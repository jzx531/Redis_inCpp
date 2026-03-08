#ifndef HASHTABLES_H
#define HASHTABLES_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>


// 哈希表节点，应该嵌入到有效载荷中
struct HNode {
    HNode *next = NULL;
    uint64_t hcode = 0;
};

// 一个简单的固定大小的哈希表
struct HTab {
    HNode **tab = NULL;
    size_t mask = 0;
    size_t size = 0;
};

// 真正的哈希表接口。
// 它用两个哈希表来逐步调整大小。
struct HMap {
    HTab ht1;
    HTab ht2;
    size_t resizing_pos = 0;
};

// 键的结构
struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

// 键空间的数据结构。
static struct {
    HMap db;
} g_data;


HNode *hm_lookup(
    HMap *hmap, HNode * key, bool (*cmp)(HNode *, HNode *));

void hm_insert(HMap *hmap, HNode *node);

HNode *hm_pop(
    HMap *hmap, HNode * key, bool (*cmp)(HNode *, HNode *));
    
void hm_clear(HMap *hmap);

#endif // HASHTABLES_H

