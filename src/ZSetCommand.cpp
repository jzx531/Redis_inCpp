#include "ZSetCommand.h"

#include <math.h>

// const size_t k_max_msg = 4096;

static void send_msg(const std::string &msg,uint8_t *res,uint32_t *reslen) {
    int len = msg.size();
    memcpy(res,msg.data(),len);
    *reslen = len;
}


static ZSetEntry * zsetentry_new(uint32_t type){
    ZSetEntry * entry = new ZSetEntry();
    // entry->zset = zset_new();
    entry->type = type;
    return entry;
}

static void entry_set_ttl(ZSetEntry *ent, int64_t ttl_ms)
{
    if (ttl_ms <= 0 && ent->heap_idx !=(size_t)-1)
    {
        //从堆里删除一个元素
        //用数组里最后一个元素替换它
        size_t pos = ent->heap_idx;
        g_data.heap[pos] = g_data.heap.back();
        g_data.heap.pop_back();
        if(pos < g_data.heap.size()){
            heap_update(g_data.heap.data(), pos, g_data.heap.size());
        }
        ent->heap_idx = (size_t)-1;
    }else if(ttl_ms >= 0){
        size_t pos = ent->heap_idx;
        if(pos == (size_t)-1){
            //向堆里添加一个新元素
            HeapItem item;
            item.ref = &ent->heap_idx;
            g_data.heap.push_back(item);
            pos = g_data.heap.size()-1;
        }
        g_data.heap[pos].val = get_monotonic_usec() + (uint64_t)ttl_ms * 1000;
        heap_update(g_data.heap.data(), pos, g_data.heap.size());
        printf("g_data.heap.size() = %d\n",g_data.heap.size());
    }
}

/*
void entry_del(ZSetEntry *ent) {
    if (ent->type == T_ZSET) {
        zset_clear(ent->zset);
    }
    //删除Entry时,将可能存在的TTL定时器也删掉
    entry_set_ttl(ent, -1);
    delete ent;
}*/
// 使用线程池版本的entry_del
// 立即释放键
static void entry_destroy(ZSetEntry *ent)  {
    switch  (ent->type)  {
        case  T_ZSET:
            zset_clear(ent->zset);
            break;
    }
    delete  ent;
}

static void entry_del_async(void *arg)  {
    entry_destroy((ZSetEntry * )arg);
}

// 从键空间分离后释放entry
void entry_del(ZSetEntry *ent)  {
    entry_set_ttl(ent,  -1);

    const size_t k_large_container_size =   10000;
    bool too_big =  false;
    switch  (ent->type)  {
        case  T_ZSET:
            too_big =  hm_size(&ent->zset->hmap)  >  k_large_container_size;
            break;
    }

    if  (too_big)  {
        thread_pool_queue(&g_data.tp,  &entry_del_async,  ent);
    }  else  {
        entry_destroy(ent);
    }
}



//比较HNode
bool zsetentry_eq(HNode *node ,HNode *key)
{
   struct ZSetEntry *ent = container_of(node, ZSetEntry, node);
   struct LookupKey *lk = container_of(key, LookupKey, node);
   return ent->key == lk->key;
}


uint32_t do_ZSet_set(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    // 查找node
    HNode *node = hm_lookup(&g_data.db,&key.node, &zsetentry_eq);

    // 如果node存在，则更新
    if (node) {
        struct ZSetEntry *ent = container_of(node, ZSetEntry, node);
        if (ent->type != T_STR) {
            const char* msg = "NotString";
            size_t len = strlen(msg);
            memcpy(res, msg, len);
            *reslen = (uint32_t)len;
            return RES_ERR;
        }
        ent->val.swap(cmd[2]);
    }else{
        // 创建新的node
        ZSetEntry *ent = zsetentry_new(T_STR);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        ent->val.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }

    const char* msg = "OK";
    size_t len = strlen(msg);
    memcpy(res, msg, len);
    *reslen = (uint32_t)len;

    return RES_OK;
}

uint32_t do_ZSet_get(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    // 查找node
    HNode *node = hm_lookup(&g_data.db, &key.node, &zsetentry_eq);
    if(!node){
        return RES_NX;
    }
    const std::string& value = container_of(node, ZSetEntry, node)->val;
    assert(value.size() < k_max_msg);
    memcpy(res, value.data(), value.size());
    *reslen = value.size();
    return RES_OK;
}

uint32_t do_ZSet_del(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    // 查找node
    HNode *node = hm_pop(&g_data.db,&key.node, &zsetentry_eq);
    if(!node){
        send_msg("Not found", res, reslen);
        return RES_NX;
    }
    entry_del(container_of(node, ZSetEntry, node));

    send_msg("OK", res, reslen);
    return RES_OK;
}

static void out_str(std::string &out, const std::string &val)  {
    // out.push_back(SER_STR);
    uint32_t len = (uint32_t)val.size();
    out.append((char *)&len, 4);
    out.append(val);
}

static void cb_scan(HNode *node, void *arg){
    std::string &out = *(std::string *)arg;
    out_str(out, container_of(node, ZSetEntry, node)->key);
}

uint32_t do_ZSet_keys(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    (void)cmd;
    // 1. 执行扫描
    // 建议先检查 db 是否初始化
    if (g_data.db.ht1.size == 0 && g_data.db.ht2.size == 0) {
        *reslen = 0;
        return RES_OK; // 空列表也是成功
    }
    std::string out;
    out.reserve(k_max_msg);
    h_scan(&g_data.db.ht1, &cb_scan, &out);
    h_scan(&g_data.db.ht2, &cb_scan, &out);

    memcpy(res, out.data(), out.size());
    *reslen = (uint32_t)out.size();

    return SER_STR;
}

static bool str2dbl(const std::string &s, double &out) {
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
}

static bool str2int(const std::string &s, int64_t &out) {
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
}

uint32_t do_zadd(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    double score = 0;
    if (!str2dbl(cmd[2], score)) {
        send_msg("Expcet Float", res, reslen);
        return RES_ERR;
    }
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &zsetentry_eq);
    //用g_data.db存储zset
    ZSetEntry * ent = NULL;
    if (!hnode) {
        ent = zsetentry_new(T_ZSET);
        ent->zset = new ZSet();
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&g_data.db, &ent->node);
    } else {
        ent = container_of(hnode, ZSetEntry, node);
        if (ent->type != T_ZSET) {
            send_msg("Not ZSet", res, reslen);
            return RES_ERR;
        }
    }
  
    const std::string& name = cmd[3];
    bool added = zset_add(ent->zset, name.data(), name.size(), score);
    // printf("added:%d\n", added);
    if (added) {
        send_msg("OK", res, reslen);
        return RES_OK;
    }
    send_msg("added", res, reslen);
    return RES_ERR;
}

static const ZSet k_empty_zset;

static ZSet* expect_zset(std::string &s) {
    LookupKey key;
    key.key.swap(s);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &zsetentry_eq);
    if (!hnode) {   // a non-existent key is treated as an empty zset
        return (ZSet *)&k_empty_zset;
    }
    ZSetEntry *ent = container_of(hnode, ZSetEntry, node);
    return ent->type == T_ZSET ? ent->zset : NULL;
}

uint32_t do_zrem(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    ZSet *zset = expect_zset(cmd[1]);
    //在gdata.db中查找zset
    if (!zset) {
        send_msg("Not ZSet", res, reslen);
        return RES_ERR;
    }
    const std::string& val = cmd[2];
    // 在zset的hmap查找val
    ZNode *znode = zset_lookup(zset, val.data(), val.size());
    if(znode){
        zset_delete(zset, znode);
        send_msg("OK", res, reslen);
        return RES_OK;
    }
    else{
        send_msg("Not found", res, reslen);
        return RES_NX;
    }
}

uint32_t do_zscore(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    ZSet* zset = expect_zset(cmd[1]);
    if(!zset){
        send_msg("Not ZSet", res, reslen);
        return RES_ERR;
    }
    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    if(znode){
        double score = znode->score;
        std::string score_str = std::to_string(score);
        send_msg(score_str, res, reslen);
    }else{
        send_msg("Not found", res, reslen);
        return RES_NX;
    }
    return RES_OK;
}

uint32_t do_zquery(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    //parse args
    double score = 0;
    if(!str2dbl(cmd[2], score))
    {
        send_msg("Expcet Float", res, reslen);
        return RES_ERR;
    }
    const std::string &name = cmd[3];
    int64_t offset = 0, limit = 0;
    if (!str2int(cmd[4], offset) || !str2int(cmd[5], limit))
    {
        send_msg("Expcet Int", res, reslen);
        return RES_ERR;
    }
    
    ZSet *zset = expect_zset(cmd[1]);
    if (!zset) {
        send_msg("Not ZSet", res, reslen);
        return RES_ERR;
    }
    if(limit  < 0){
        send_msg("Limit should be positive", res, reslen);
        return RES_ERR;
    }
    ZNode *znode = zset_query(zset, score, name.data(), name.size(), offset);
    //output
    int64_t n=0;
    std::string out;
    while(znode && limit > n){
        // printf("%s %f\n", znode->name, znode->len);
        std::string name_str ;
        name_str.append(znode->name, znode->len);
        out_str(out, name_str);
        // printf("%s", znode->name[0]);
        std::string score_str = std::to_string(znode->score);
        out_str(out,score_str);
        znode = znode_offset(znode, 1);
        n+=2;
    }
    memcpy(res, out.data(), out.size());
    *reslen = (uint32_t)out.size();

    // for(auto c : out){
    //     printf("%c", c);
    // }
    // printf("%d\n", *reslen);
    return ZQUERY_RES;   
}

uint32_t do_expire(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    int64_t ttl_ms =  0;
    if (!str2int(cmd[2], ttl_ms)) {
        send_msg("Expcet Int", res, reslen);
        return RES_ERR;
    }
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &zsetentry_eq);
    if (!hnode) {
        send_msg("Not found", res, reslen);
        return RES_NX;
    }
    ZSetEntry *ent = container_of(hnode, ZSetEntry, node);
    entry_set_ttl(ent, ttl_ms);
    send_msg("OK", res, reslen);
    return RES_OK;
}

uint32_t do_ttl(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen)
{
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &zsetentry_eq);
    if (!hnode) {
        send_msg("-2", res, reslen);
        return RES_NX;
    }
    ZSetEntry *ent = container_of(hnode, ZSetEntry, node);
    if (ent->heap_idx == -1) {
        send_msg("-1", res, reslen);
        return RES_NX;
    }
    int64_t ttl_ms = (int64_t)(g_data.heap[ent->heap_idx].val - get_monotonic_usec()) / 1000;
    if (ttl_ms < 0) {
        send_msg("-1", res, reslen);
        return RES_OK;
    }
    std::string ttl_str = std::to_string(ttl_ms);
    send_msg(ttl_str, res, reslen);
    return RES_OK;
}
