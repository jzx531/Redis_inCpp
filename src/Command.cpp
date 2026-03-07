#include "Command.h"

const size_t k_max_msg = 4096;
//先使用std::map作为存储结构，后续替代为其他数据结构
static std::map<std::string,std::string> g_map;



uint32_t do_get(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen){
        // printf("get cmd\n");
        //找不到键值，返回NX
        if(!g_map.count(cmd[1])){
            return RES_NX;
        }
        std::string value = g_map[cmd[1]];
        assert(value.size() < k_max_msg);
        memcpy(res, value.data(), value.size());
        *reslen = value.size();
        return RES_OK;
    }

uint32_t do_set(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen){

        // printf("set cmd\n");

        const char* msg = "OK";
        size_t len = strlen(msg);
        
        // 1. 拷贝数据到缓冲区
        memcpy(res, msg, len);
        *reslen = (uint32_t)len;

        std::string value = cmd[2];
        assert(value.size() < k_max_msg);
        g_map[cmd[1]] = value;

        return RES_OK;
    }

uint32_t do_del(
    const std::vector<std::string>  &cmd, 
    uint8_t * res, 
    uint32_t * reslen){
        // printf("del cmd\n");

        if(!g_map.count(cmd[1])){
            const char* msg = "Fail";
            size_t len = strlen(msg);
        
            // 1. 拷贝数据到缓冲区
            memcpy(res, msg, len);
            *reslen = (uint32_t)len;
            return RES_NX;
        }
        g_map.erase(cmd[1]);
        const char* msg = "OK";
        size_t len = strlen(msg);
        // 1. 拷贝数据到缓冲区
        memcpy(res, msg, len);
        *reslen = (uint32_t)len;
        return RES_OK;
    }

    