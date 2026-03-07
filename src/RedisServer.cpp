#include "RedisServer.h"
#include "Command.h"
#include "util.h"
#include "HashCommand.h"

#include <vector>
#include <cstdint>
#include <cassert>
#include <sys/epoll.h>
#include <memory>
#include <algorithm>
#include <cctype> // for tolower
#include <cstring> // for strlen

const size_t k_max_msg =  4096;
const size_t k_max_args = 10;
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
};

// --- 全局连接管理 (fd -> Conn*) ---
// 生产环境建议使用 hash map，这里为了简单沿用 vector
static std::vector<Conn*> fd2conn;

static void conn_put(int fd, Conn* conn) {
    if (fd2conn.size() <= (size_t)fd) {
        fd2conn.resize(fd + 1, nullptr);
    }
    fd2conn[fd] = conn;
}

char errnobuf[k_max_msg];

// --- 核心逻辑：接受新连接 ---
static void accept_new_conn(int lfd, int epollfd) {
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    
    // 注意：在 ET 模式下，必须循环 accept 直到返回 EAGAIN
    while (true) {
        int connfd = accept(lfd, (struct sockaddr*)&client_addr, &socklen);
        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 没有更多连接了
            }
            msg("accept() error");
            break;
        }

        // 设置非阻塞
        fd_set_nb(connfd);

        // 创建 Conn 对象
        Conn* conn = new Conn();
        conn->fd = connfd;
        conn->state = STATE_REQ;
        conn->rbuf_size = 0;
        conn->wbuf_size = 0;
        conn->wbuf_sent = 0;

        conn_put(connfd, conn);

        // 注册到 epoll (监听读事件，使用 ET 模式)
        struct epoll_event ev = {};
        ev.events = EPOLLIN | EPOLLET; 
        ev.data.ptr = conn; // 直接绑定 Conn 指针，避免二次查找
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev) < 0) {
            die("epoll_ctl ADD");
        }
        
        printf("New connection: fd=%d\n", connfd);
    }
}

/* 下面的process_request,process_response使用用于回声echo服务器的*/
/*--------------------echo server start----*/
/*
// --- 核心逻辑：处理读事件 (解析协议) ---
static bool process_request(Conn* conn,int epollfd) {
    // 循环读取，直到 EAGAIN (ET 模式要求一次读完)
    while (true) {
        assert(conn->rbuf_size < sizeof(conn->rbuf));
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        
        ssize_t n = read(conn->fd, conn->rbuf + conn->rbuf_size, cap);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 数据读完了
            }
            msg("read() error");
            return false; // 出错，关闭连接
        }
        if (n == 0) {
            msg("EOF (client closed)");
            return false; // 客户端关闭
        }
        
        conn->rbuf_size += n;
    }

    // 处理缓冲区中的数据 (支持流水线/pipelining)
    while (conn->rbuf_size >= 4) {
        uint32_t len = 0;
        memcpy(&len, conn->rbuf, 4);
        
        if (len > k_max_msg) {
            msg("Message too long");
            return false;
        }
        
        if (conn->rbuf_size < 4 + len) {
            break; // 数据包不完整，等待更多数据
        }

        // --- 业务逻辑：回显 ---
        printf("Received: %.*s\n", len, conn->rbuf + 4);

        // 构造响应 (长度 + 内容)
        memcpy(conn->wbuf, &len, 4);
        memcpy(conn->wbuf + 4, conn->rbuf + 4, len);
        conn->wbuf_size = 4 + len;
        conn->wbuf_sent = 0;

        // 移除已处理的请求 (简单的 memmove，生产环境可用环形缓冲区优化)
        size_t remain = conn->rbuf_size - (4 + len);
        if (remain > 0) {
            memmove(conn->rbuf, conn->rbuf + 4 + len, remain);
        }
        conn->rbuf_size = remain;

        // 切换到写状态
        conn->state = STATE_RES;
        
        // 修改 epoll 监听为 写事件 (POLLOUT)
        struct epoll_event ev = {};
        ev.events = EPOLLOUT | EPOLLET;
        ev.data.ptr = conn;
        if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) { // 注意：第一个参数是 epollfd，这里需要传进去，稍后修复
             // 这里的逻辑需要在主循环中处理，或者把 epollfd 传进来
             // 为了简化，我们在主循环的状态判断中统一修改事件，或者在这里返回状态让主循环改
                die("epoll_ctl MOD to RES");
            }
    }
    return true;
}
*/
// --- 核心逻辑：处理写事件 (发送响应) ---
static bool process_response(Conn* conn, int epollfd) {
    while (conn->wbuf_sent < conn->wbuf_size) {
        
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        ssize_t n = write(conn->fd, conn->wbuf + conn->wbuf_sent, remain);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true; // 发送缓冲区满，下次再试
            }
            msg("write() error");
            return false;
        }
        
        conn->wbuf_sent += n;
    }

    // 发送完毕，切回读状态
    conn->state = STATE_REQ;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;

    // 修改 epoll 监听回 读事件 (POLLIN)
    struct epoll_event ev = {};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = conn;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) {
        die("epoll_ctl MOD");
    }
    
    return true;
}
/*--------------------echo server end---------------------------*/

// --- 关闭连接 ---
static void close_conn(int epollfd, Conn* conn) {
    if (conn->fd >= 0) {
        epoll_ctl(epollfd, EPOLL_CTL_DEL, conn->fd, NULL);
        close(conn->fd);
        if ((size_t)conn->fd < fd2conn.size()) {
            fd2conn[conn->fd] = nullptr;
        }
        delete conn;
        printf("Connection closed\n");
    }
}

static bool cmd_is(const std::string& cmd, const char* name) {
    const char * cmd_name = (char*)cmd.c_str();
    size_t len = strlen(name);
    if (len != cmd.length()) {
        return false;
    }
    // 2. 逐个字符忽略大小写比较
    for (size_t i = 0; i < len; ++i) {
        // 转为小写后比较
        if (tolower((unsigned char)cmd_name[i]) != tolower((unsigned char)name[i])) {
            return false;
        }
    }
    return true;
}
/* 以下为处理redis-client请求的函数*/
/**
 * 解析命令
 * @param data
 * @param len
 * @param out
 * @return int32_t
 */
static int32_t parse_req(
    const uint8_t *data, size_t len, std::vector<std::string>  &out)
    {
        if (len < 4) {
            return -1;
        }
        uint32_t n = 0;
        memcpy(&n,&data[0],4);
        if(n > k_max_args){
            return -1;
        }
        
        // n为字符串数量,每个字符串对为 len + str
        size_t pos = 4;
        while(n--){
            if(pos + 4 > len){//没能构成完整的字符串对，返回错误
                return -1;
            }
            //获取字符串对中的len
            uint32_t str_len = 0;
            memcpy(&str_len,&data[pos],4);
            if(pos + 4 + str_len > len){//没能构成完整的指令，返回错误
                return -1;
            }
            // 将质量内容压入vector
            out.push_back(std::string((const char*)(data + pos + 4), str_len));
            pos += 4 + str_len;
        }
 
        if (pos != len) {
            return -1;   // 有多余的无用数据
        }
        return 0;
    }

    //解析收到的指令，并执行相应的操作
static int32_t deal_req(
    const uint8_t * req, uint32_t reqlen,
    uint32_t * rescode, uint8_t * res, uint32_t * reslen)
{
    std::vector<std::string> cmd;
    if (0 != parse_req(req, reqlen, cmd)) {
        msg("bad req");
        return -1;
    }

    
    // 在解析 cmd 后清理
    for (auto &s : cmd) {
        while (!s.empty() && (s.back() == '\r' || s.back() == '\n')) {
            s.pop_back();
        }
    }
    
    if (cmd.size() == 2 && cmd_is(cmd[0], "get")) {
        // *rescode = do_get(cmd, res, reslen);
        *rescode = do_HashMap_get(cmd, res, reslen);
    } else if (cmd.size() == 3 && cmd_is(cmd[0], "set")) {
        // *rescode = do_set(cmd, res, reslen);
        *rescode = do_HashMap_set(cmd, res, reslen);
    } else if (cmd.size() == 2 && cmd_is(cmd[0], "del")) {
        // *rescode = do_del(cmd, res, reslen);
        *rescode = do_HashMap_del(cmd, res, reslen);
    } else {
        // 不识别的命令
        *rescode = RES_ERR;
        const char *msg = "Unknown cmd";
        strcpy((char * ) res, msg);
        *reslen = strlen(msg);
        return 0;
    }
    return 0;
}


static bool process_request(Conn* conn,int epollfd) {
    // 循环读取，直到 EAGAIN (ET 模式要求一次读完)
    while (true) {
        assert(conn->rbuf_size < sizeof(conn->rbuf));
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        
        ssize_t n = read(conn->fd, conn->rbuf + conn->rbuf_size, cap);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 数据读完了
            }
            msg("read() error");
            return false; // 出错，关闭连接
        }
        if (n == 0) {
            msg("EOF (client closed)");
            return false; // 客户端关闭
        }
        
        conn->rbuf_size += n;
    }

    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg) {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }

    //拿到一个请求,生成相应
    uint32_t rescode = 0;
    uint32_t wlen = 0;
    int32_t err = deal_req(
        &conn->rbuf[4], len,
        &rescode, &conn->wbuf[4+4], &wlen
    );

    if (err) {
        // printf("deal_req error\n");
        conn->state = STATE_END;
        return false;
    }
    // printf("deal_req success\n");
    // 协议格式: [TotalLen:4] [ResCode:4] [Data:...]
    // TotalLen = 4 (ResCode) + data_len
    uint32_t total_content_len = 4 + wlen;

    // 写入头部
    memcpy(&conn->wbuf[0], &total_content_len, 4);
    memcpy(&conn->wbuf[4], &rescode, 4);
    // Data 已经在 deal_req 中写入到 wbuf[8...]

    conn->wbuf_size = 4 + total_content_len; // 4字节头 + 内容总长
    conn->wbuf_sent = 0;

    // printf("Response: content_len=%u (code=4 + data=%u), total_send=%zu, code=%u\n", 
    //        total_content_len, wlen, conn->wbuf_size, rescode);
    // 从缓冲区移除请求
    // 注意：频繁使用memmove效率不高
    // 注意：生产环境代码需要更好的处理方式
    size_t remain = conn->rbuf_size - (4 + len);
    if (remain > 0) {
        memmove(conn->rbuf, conn->rbuf + 4 + len, remain);
    }
    conn->rbuf_size = remain;

    //切换到写状态
    conn->state = STATE_RES;

    // 修改epoll监听为写事件
    struct epoll_event ev = {};
    ev.events = EPOLLOUT | EPOLLET;
    ev.data.ptr = conn;
    if (epoll_ctl(epollfd, EPOLL_CTL_MOD, conn->fd, &ev) < 0) { // 注意：第一个参数是 epollfd，这里需要传进去，稍后修复
        // 这里的逻辑需要在主循环中处理，或者把 epollfd 传进来
        // 为了简化，我们在主循环的状态判断中统一修改事件，或者在这里返回状态让主循环改
        die("epoll_ctl MOD to RES");
    }
    return true;
}


// --- 服务器主函数 ---
void RedisServer() {
    // 1. 创建监听 socket
    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) die("socket");
    
    int val = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
    fd_set_nb(lfd); // 监听 fd 也建议设为非阻塞

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234); // 修正：应该用 htons 而不是 ntohs
    addr.sin_addr.s_addr = htonl(0);

    if (bind(lfd, (sockaddr*)&addr, sizeof(addr)) < 0) die("bind");
    if (listen(lfd, SOMAXCONN) < 0) die("listen");

    printf("Server listening on port 1234...\n");

    // 2. 创建 epoll 实例
    int epollfd = epoll_create1(0);
    if (epollfd < 0) die("epoll_create1");

    // 3. 注册监听 fd
    struct epoll_event ev = {};
    ev.events = EPOLLIN | EPOLLET;
    ev.data.ptr = nullptr; // 监听 fd 没有对应的 Conn，特殊处理
    // 我们用一个技巧：如果是监听 fd，data.fd 存 lfd，或者单独判断
    // 这里我们简单点：data.fd 存 lfd，通过判断 fd == lfd 来区分
    ev.data.fd = lfd; 
    
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, lfd, &ev) < 0) die("epoll_ctl ADD lfd");

    // 4. 事件循环
    const int MAX_EVENTS = 1024;
    std::vector<struct epoll_event> events(MAX_EVENTS);

    while (true) {
        int n = epoll_wait(epollfd, events.data(), MAX_EVENTS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            die("epoll_wait");
        }

        for (int i = 0; i < n; ++i) {
            struct epoll_event& e = events[i];
            int fd = e.data.fd;
            Conn* conn = (Conn*)e.data.ptr;

            // 情况 A: 监听 socket 有新连接
            if (fd == lfd) {
                accept_new_conn(lfd, epollfd);
                continue;
            }

            // 情况 B: 客户端连接事件
            if (!conn) {
                sprintf(errnobuf, "Unexpected event for fd %d", fd);
                msg(errnobuf);
                continue;
            }

            bool keep_conn = true;

            // 处理读事件
            if (e.events & EPOLLIN) {
                if (conn->state != STATE_REQ) {
                    // 状态不一致，可能是残留事件，忽略或报错
                    continue; 
                }
                printf("fd=%d read event\n", fd);
                keep_conn = process_request(conn,epollfd);
                // process_request 内部如果生成了响应，会切换 state 为 RES
                // 但我们需要手动更新 epoll 监听类型
                if (keep_conn && conn->state == STATE_RES) {
                    struct epoll_event mod_ev = {};
                    mod_ev.events = EPOLLOUT | EPOLLET;
                    mod_ev.data.ptr = conn;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &mod_ev);
                }
            }

            // 处理写事件
            if (e.events & EPOLLOUT) {
                if (conn->state != STATE_RES) {
                    continue;
                }
                keep_conn = process_response(conn, epollfd);
                // process_response 内部已经处理了切回 REQ 和 epoll MOD
            }

            // 处理错误/挂起
            if (e.events & (EPOLLERR | EPOLLHUP)) {
                keep_conn = false;
            }

            if (!keep_conn) {
                close_conn(epollfd, conn);
            }
        }
    }
}

