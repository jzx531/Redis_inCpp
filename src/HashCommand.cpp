#include "HashCommand.h"
#include "Command.h"

const size_t k_max_msg = 4096;
//通过node查询key和val
/*
    下面的宏先通过__typeof__(  ((type *)0)->member )  *     mptr =  (ptr)
    定义一个member类型的指针mptr，并将其初始化为传入的ptr参数。
    然后通过(char *)mptr - offsetof(type, member)计算出包含该成员的结构体的起始地址，并将其转换为type*类型返回。
    将指针转为字节指针确保计算准确性
    offsetof(type, member)  计算出member在type结构体中的偏移量，以字节为单位。
*/
#define container_of(ptr, type, member)  ({                                         \
    const __typeof__(  ((type *)0)->member )  *     mptr =  (ptr);        \
    (type *)(  (char *)      mptr -  offsetof(type, member)  );})

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    
    bool code_match = (lhs->hcode == rhs->hcode);
    bool key_match = (le->key == re->key);
    
    // 临时调试输出
    if (!code_match || !key_match) {
        fprintf(stderr, "DEBUG: Match Failed!\n");
        fprintf(stderr, "  LHS Hash: %u, RHS Hash: %u\n", (unsigned)lhs->hcode, (unsigned)rhs->hcode);
        fprintf(stderr, "  LHS Key: '%s' (len=%zu)\n", le->key.c_str(), le->key.size());
        fprintf(stderr, "  RHS Key: '%s' (len=%zu)\n", re->key.c_str(), re->key.size());
        // 打印十六进制查看是否有隐藏字符
        fprintf(stderr, "  RHS Key Hex: ");
        for(char c : re->key) fprintf(stderr, "%02x ", (unsigned char)c);
        fprintf(stderr, "\n");
    }
    
    return code_match && key_match;
}

// 原型定义
// 修改 hash 函数返回类型以匹配
uint32_t str_hash(const uint8_t *data, size_t len) {
    uint32_t hash = 5381;
    for (size_t i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
}

uint32_t do_HashMap_set(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db,&key.node, &entry_eq);
    if(node){
        container_of(node, Entry, node)->val.swap(cmd[2]);
    }else{
        Entry *entry = new Entry();
        entry->key.swap(key.key);
        entry->node.hcode = key.node.hcode;
        entry->val.swap(cmd[2]);
        hm_insert(&g_data.db, &entry->node);
    }

    const char* msg = "OK";
    size_t len = strlen(msg);
    // 1. 拷贝数据到缓冲区
    memcpy(res, msg, len);
    *reslen = (uint32_t)len;

    return RES_OK;
}

uint32_t do_HashMap_get(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db,&key.node, &entry_eq);
    if(!node){
        return RES_NX;
    }

    const std::string& value = container_of(node, Entry, node)->val;
    assert(value.size() < k_max_msg);
    memcpy(res, value.data(), value.size());
    *reslen = value.size();
    return RES_OK;
}

uint32_t do_HashMap_del(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_pop(&g_data.db,&key.node, &entry_eq);
    if(!node){
        const char* msg = "Fail";
        size_t len = strlen(msg);
        // 1. 拷贝数据到缓冲区
        memcpy(res, msg, len);
        *reslen = (uint32_t)len;
        return RES_NX;
    }
    delete container_of(node, Entry, node);

    const char* msg = "OK";
    size_t len = strlen(msg);
    // 1. 拷贝数据到缓冲区
    memcpy(res, msg, len);
    *reslen = (uint32_t)len;
    return RES_OK;
}


