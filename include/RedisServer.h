#ifndef REDISSERVER_H
#define REDISSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "DList.h"
#include "util.h"


// --- 连接状态定义 ---
enum {
    STATE_REQ = 0,  // 等待读取请求
    STATE_RES = 1,  // 等待写入响应
    STATE_END = 2   // 连接关闭
};

// --- 连接结构体 ---
struct Conn {
    int fd = -1;
    uint32_t state = STATE_REQ;
    
    // 读缓冲区
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    
    // 写缓冲区
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];

    uint64_t idle_start = 0;  // 最近一次活动时间
    //定时器
    DList idle_list;
};

void RedisServer();

#endif // REDISSERVER_H

