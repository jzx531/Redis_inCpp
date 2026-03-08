#include "RedisClient.h"
#include "Command.h"

#include <vector>
#include <cstdint>
#include <cassert>
#include <sys/epoll.h>
#include <memory>
#include <algorithm>
#include <cctype> // for tolower
#include <cstring> // for strlen
#include <iostream>
#include <sstream>

const size_t k_max_msg =  4096;

static int32_t send_req(int fd, const std::vector<std::string>  &cmd)  {
    uint32_t len = 4;
    for (const std::string &s : cmd) {
        len += 4 + s.size();
    }
    if (len > k_max_msg) {
        return -1;
    }

    char wbuf[4 + k_max_msg];
    memcpy(&wbuf[0], &len, 4);    // 假设是小端序
    uint32_t n = cmd.size();
    memcpy(&wbuf[4], &n, 4);
    size_t cur = 8;
    for (const std::string &s : cmd) {
        uint32_t p = (uint32_t)s.size();
        memcpy(&wbuf[cur], &p, 4);
        memcpy(&wbuf[cur + 4], s.data(), s.size());
        cur += 4 + s.size();
    }
    return write_all(fd, wbuf, 4 + len);
}

static int32_t read_res(int fd)  {
    char rbuf[4 + k_max_msg];
    int32_t n = read_full(fd, rbuf, 4);
    if (n < 0) {
        return -1;
    }
    uint32_t len = 0;
    memcpy(&len, &rbuf[0], 4);
    if (len > k_max_msg) {
        msg("response too long");
        return -1;
    }
// 长度至少包含 4 字节的状态码
    if (len < 4) {
        msg("Bad response: too short");
        return -1;
    }

     // 打印结果
    uint32_t rescode = 0;
    if (len < 4) {
        msg("bad response");
        return -1;
    }
    read_full(fd, rbuf + 4, len); 
    memcpy(&rescode, &rbuf[4], 4);
    if(rescode !=3){
        printf("server says: [%u] %.*s\n", rescode, len - 4, &rbuf[8]);
    }
    else{
        printf("server says: [%u]\n", rescode);
        uint32_t data_len = len - 4;
        uint32_t offset = 8;
        while(data_len > (offset-8)){
            uint32_t key_len = 0;
            memcpy(&key_len, &rbuf[offset], 4);
            offset += 4;
            if(data_len < (offset-8+key_len)){
                printf("bad response");
            }else{
                printf("key_len: %u", key_len);
                printf(" key: %.*s", key_len, &rbuf[offset]);
                offset += key_len;
            }
            printf("\n");
        }
    }
    /*
    // 2. 读取剩余部分 (状态码 + 数据)
    // 已经读了4字节，还需要读 len 字节
    n = read_full(fd, rbuf + 4, len);
    if (n < 0) return -1;

    // 3. 解析状态码 (位于偏移量 4)
    uint32_t rescode = 0;
    memcpy(&rescode, &rbuf[4], 4);

    // 4. 解析数据 (位于偏移量 8)
    // 数据长度 = len - 4
    uint32_t data_len = len - 4;
    
    // 防止打印非字符串垃圾数据，确保有结束符或限制长度
    printf("server says: [%u] ", rescode);
    if (data_len > 0) {
        // 确保不越界打印
        if (data_len > k_max_msg - 8) data_len = k_max_msg - 8;
        printf("%.*s\n", data_len, &rbuf[8]);
    } else {
        printf("(empty)\n");
    }*/

    return 0;
}

void RedisClient(){
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        die("socket()");
    }

    struct  sockaddr_in addr =  {};
    addr.sin_family =  AF_INET;
    addr.sin_port =  ntohs(1234);
    addr.sin_addr.s_addr =  ntohl(INADDR_LOOPBACK);    //  127.0.0.1
    int rv =  connect(fd,  (const struct  sockaddr * )&addr,  sizeof(addr));
    if  (rv)  {
        die("connect");
    }

std::string line;
    while (true) {
        printf("> "); // 提示符
        fflush(stdout); // 确保提示符立即显示

        // 1. 从键盘读取一行
        if (!std::getline(std::cin, line)) {
            // 读取失败或遇到 EOF (Ctrl+D)
            printf("\nExiting...\n");
            break;
        }

        // 去除行尾可能的回车符 (兼容 Windows/Linux)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 2. 检查退出命令
        if (line == "quit" || line == "exit") {
            printf("Goodbye!\n");
            break;
        }

        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 3. 解析命令行字符串到 vector<string>
        // 使用 stringstream 按空格分割
        std::vector<std::string> cmd;
        std::istringstream iss(line);
        std::string arg;
        while (iss >> arg) {
            cmd.push_back(arg);
        }

        if (cmd.empty()) {
            continue;
        }

        // 4. 发送请求
        // for(auto &s : cmd)
        // {
        //     printf("cmd: %s\n", s.c_str());
        // }

        int32_t err = send_req(fd, cmd);
        if (err) {
            msg("Failed to send request");
            break;
        }

        // 5. 接收响应
        err = read_res(fd);
        if (err) {
            msg("Failed to read response");
            break;
        }
    }
    close(fd);
}
