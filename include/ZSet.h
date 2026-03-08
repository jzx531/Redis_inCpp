#ifndef SORTEDSET_H
#define SORTEDSET_H

#include "Hashtables.h"
#include "AVLtree.h"
#include "HashCommand.h"
#include "Command.h"

struct  ZSet {
    AVLNode *tree =  NULL;
    HMap hmap;
};

struct  ZNode {
    AVLNode tree;
    HNode hmap;
    double score =  0;
    size_t len =  0;
    char name[0];
};

void tree_add(ZSet *zset,  ZNode *node);

void zset_update(ZSet *zset,  ZNode *node,  double score);

ZNode *zset_lookup(ZSet *zset,  const char *name,  size_t len);

bool zset_add(ZSet *zset,const char *name,size_t len,double score);

ZNode *zset_seekge(ZSet *zset, double score, const char *name, size_t len);

ZNode *znode_offset(ZNode *node, int64_t offset);

ZNode *zset_query(
    ZSet *zset,  double score,  const char *name,  size_t len,  int64_t offset);

void zset_delete(ZSet *zset, ZNode *node);

void zset_clear(ZSet *zset);

#endif // SORTEDSET_H

