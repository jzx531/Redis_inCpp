#ifndef HASHCOMMAND_H
#define HASHCOMMAND_H

#include "Hashtables.h"

uint32_t str_hash(const uint8_t *data, size_t len);

bool entry_eq(HNode *lhs, HNode *rhs);

uint32_t do_HashMap_get(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);


uint32_t do_HashMap_set(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

    
uint32_t do_HashMap_del(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

uint32_t do_HashMap_keys(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen
);

void h_scan(HTab * tab,void(*f)(HNode*,void*),void* arg);

#endif // HASHCOMMAND_H

