#ifndef HASHCOMMAND_H
#define HASHCOMMAND_H

#include "Hashtables.h"

uint32_t do_HashMap_get(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);


uint32_t do_HashMap_set(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);

    
uint32_t do_HashMap_del(
    std::vector<std::string>  &cmd, uint8_t * res, uint32_t * reslen);


#endif // HASHCOMMAND_H

