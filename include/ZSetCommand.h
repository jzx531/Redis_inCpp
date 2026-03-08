#ifndef ZSETCOMMAND_H
#define ZSETCOMMAND_H

#include "ZSet.h"
#include "Command.h"
#include "AVLtree.h"
#include "HashCommand.h"
#include "Hashtables.h"

// value types
enum {
    T_INIT  = 0,
    T_STR   = 1,    // string
    T_ZSET  = 2,    // sorted set
};

// 键的结构
struct  ZSetEntry {
    struct  HNode node;
    std::string key;
    std::string val;
    uint32_t type =  0;
    ZSet *zset =  NULL;
};

struct LookupKey {
    struct HNode node;  // hashtable node
    std::string key;
};

uint32_t do_ZSet_set(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_ZSet_get(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_ZSet_del(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_ZSet_keys(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_zadd(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_zscore(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_zrem(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_zquery(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);


#endif // ZSETCOMMAND_H

