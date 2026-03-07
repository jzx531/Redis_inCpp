#ifndef COMMAND_H
#define COMMAND_H

#include <string>
#include <vector>
#include <map>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

/*
编码格式：
+------+-----+------+-----+------+-----+-----+------+
| nstr | len | str1 | len | str2 | ... | len | strn |
+------+-----+------+-----+------+-----+-----+------+

这里的nstr是字符串的数量，len是紧跟其后的字符串的长度，它们都是32位整数。

+-----+---------+
| res | data... |
+-----+---------+
响应：32位的状态码，后面跟着响应字符串
*/


enum {
    RES_OK = 0,
    RES_ERR = 1,
    RES_NX = 2,
};

uint32_t do_get(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen);

uint32_t do_set(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen);

uint32_t do_del(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen);

#endif

