#include "PollServer.h"
#include "util.h"

#include <cstdio>      // printf, perror (对应 msg/die 宏可能用到的)
#include <cstdlib>     // malloc, free, exit (对应 die 宏)
#include <cstring>     // memcpy, memmove, memset, strerror
#include <cstdint>     // uint32_t, uint8_t, size_t (虽然某些编译器会间接包含，但标准写法需要这个)
#include <unistd.h>    // read, write, close, sleep
#include <fcntl.h>     // fcntl (用于 fd_set_nb 设置非阻塞)
#include <sys/socket.h>// socket, bind, listen, accept, setsockopt, sockaddr_in, AF_INET, SOCK_STREAM, SOMAXCONN
#include <netinet/in.h>// sockaddr_in, IPPROTO_TCP 等网络结构体
#include <arpa/inet.h> // ntohs, ntohl, inet_pton 等地址转换函数
#include <sys/poll.h>  // poll, struct pollfd, POLLIN, POLLOUT, POLLERR
#include <errno.h>     // errno, EAGAIN, EINTR
#include <cassert>     // assert
#include <vector>      // std::vector (你代码里已经有了)

const size_t k_max_msg =  4096;
// static int errno;

enum {
    STATE_REQ = 0,
    STATE_RES = 1,
    STATE_END = 2,   // 标记这个连接，准备删除它
};

struct Conn {
    int fd = -1;
    uint32_t state = 0;           // 取值为STATE_REQ 或 STATE_RES
    // 读缓冲区
    size_t rbuf_size = 0;
    uint8_t rbuf[4 + k_max_msg];
    // 写缓冲区
    size_t wbuf_size = 0;
    size_t wbuf_sent = 0;
    uint8_t wbuf[4 + k_max_msg];
};



static void conn_put(std::vector<Conn *> &fd2conn, struct Conn *conn)  {
    if (fd2conn.size() <= (size_t)conn->fd)  {
        fd2conn.resize(conn->fd + 1);
    }
    fd2conn[conn->fd] = conn;
}

static int32_t accept_new_conn(std::vector<Conn *> &fd2conn, int fd)  {
    // 接受连接
    struct sockaddr_in client_addr = {};
    socklen_t socklen = sizeof(client_addr);
    int connfd = accept(fd, (struct sockaddr *)&client_addr, &socklen);
    if (connfd < 0)  {
        msg("accept() error");
        return -1;   // 出错了
    }

    // 将新连接的文件描述符设置为非阻塞模式
    fd_set_nb(connfd);
    // 创建Conn结构体
    struct Conn *conn = (struct Conn *)malloc(sizeof(struct Conn));
    if (!conn)  {
        close(connfd);
        return -1;
    }
    conn->fd = connfd;
    conn->state = STATE_REQ;
    conn->rbuf_size = 0;
    conn->wbuf_size = 0;
    conn->wbuf_sent = 0;
    conn_put(fd2conn, conn);
    return 0;
}

static bool try_flush_buffer(Conn *conn)  {
    ssize_t rv = 0;
    {
        size_t remain = conn->wbuf_size - conn->wbuf_sent;
        rv = write(conn->fd, &conn->wbuf[conn->wbuf_sent], remain);
        if ( rv < 0 && errno == EAGAIN)  {
            // 遇到EAGAIN，停止写入
            return false;
        }
        if ( rv < 0)  {
            msg("write() error");
            conn->state = STATE_END;
            return false;
        }
        conn->wbuf_sent += (size_t)rv;
        assert(conn->wbuf_sent <= conn->wbuf_size);
        if (conn->wbuf_sent == conn->wbuf_size)  {
            // 响应全部发送完毕，切换回STATE_REQ状态
            conn->state = STATE_REQ;
            conn->wbuf_sent = 0;
            conn->wbuf_size = 0;
            return false;
        }
        // 写缓冲区还有数据，可以再试试写入
        return true;
    }
}

static void state_res(Conn *conn)  {
    while (try_flush_buffer(conn))  {}
}


static bool try_one_request(Conn *conn)  {
    // 尝试从缓冲区解析出一个请求
    if (conn->rbuf_size < 4)  {
        // 缓冲区数据不够，下次循环再试试
        return false;
    }
    uint32_t len = 0;
    memcpy(&len, &conn->rbuf[0], 4);
    if (len > k_max_msg)  {
        msg("too long");
        conn->state = STATE_END;
        return false;
    }
    if (4 + len > conn->rbuf_size)  {
        // 缓冲区数据不够，下次循环再试试
        return false;
    }

    // 拿到一个请求，处理一下
    // 控制字符串打印的长度（精度），正确的格式符是 %.*s
    printf("client says: %.*s\n", len, &conn->rbuf[4]);

    // 生成回显响应
    memcpy(&conn->wbuf[0], &len, 4);
    memcpy(&conn->wbuf[4], &conn->rbuf[4], len);
    conn->wbuf_size = 4 + len;

    // 从缓冲区移除这个请求
    // 注意：频繁调用memmove效率可不高
    // 注意：生产环境的代码得优化下这部分
    size_t remain = conn->rbuf_size - 4 - len;
    if (remain)  {
        memmove(conn->rbuf, &conn->rbuf[4 + len], remain);
    }
    conn->rbuf_size = remain;

    // 切换状态
    conn->state = STATE_RES;
    state_res(conn);

    // 如果请求处理完了，就继续外层循环
    return (conn->state == STATE_REQ);
}


static bool try_fill_buffer(Conn *conn)  {
    // 尝试填充缓冲区
    assert(conn->rbuf_size < sizeof(conn->rbuf));
    ssize_t rv = 0;
    do  {
        size_t cap = sizeof(conn->rbuf) - conn->rbuf_size;
        rv = read(conn->fd, &conn->rbuf[conn->rbuf_size], cap);
    } while ( rv < 0 && errno == EINTR);
    if ( rv < 0 && errno == EAGAIN)  {
        // 遇到EAGAIN，停止读取
        return false;
    }
    if ( rv < 0)  {
        msg("read() error");
        conn->state = STATE_END;
        return false;
    }
    if ( rv == 0)  {
        if (conn->rbuf_size > 0)  {
            msg("unexpected EOF");
        } else  {
            msg("EOF");
        }
        conn->state = STATE_END;
        return false;
    }

    conn->rbuf_size += (size_t)rv;
    assert(conn->rbuf_size <= sizeof(conn->rbuf) - conn->rbuf_size);

    // 尝试逐个处理请求
    // 为啥这里有个循环呢？去看看“流水线（pipelining）”的解释就懂啦
    //将读缓冲区的数据全部放入写缓冲区,然后将状态设置为发送等待写缓冲区发送完毕
    while (try_one_request(conn))  {}
    return (conn->state == STATE_REQ);
}

static void state_req(Conn *conn)  {
    while (try_fill_buffer(conn))  {}
}


static void connection_io(Conn *conn)  {
    if (conn->state == STATE_REQ)  {
        state_req(conn);
    } else if (conn->state == STATE_RES)  {
        state_res(conn);
    } else  {
        assert(0);    // 不该出现这种情况
    }
}


void PollServer(){
    //创建套接字文件描述符
    int  fd  =  socket(AF_INET,  SOCK_STREAM,  0);
    //设置套接字选项
    int val =  1;
    setsockopt(fd,  SOL_SOCKET,  SO_REUSEADDR,  &val,  sizeof(val));
    //  bind, 这是处理IPv4地址的语法
    struct  sockaddr_in addr =  {};
    addr.sin_family =  AF_INET;
    addr.sin_port =  ntohs(1234);
    addr.sin_addr.s_addr =  ntohl(0);       // 通配地址0.0.0.0
    int rv =  bind(fd,   (const sockaddr * )&addr,  sizeof(addr));
    if  (rv)  {
        die("bind()");
    }
    //  listen
    rv =  listen(fd,  SOMAXCONN);
    if  (rv)  {
        die("listen()");
    }
    // 一个存放所有客户端连接的映射，用文件描述符（fd）作为键
    std::vector<Conn *> fd2conn;

    // 将监听的文件描述符设置为非阻塞模式
    fd_set_nb(fd);
    // 事件循环
    std::vector<struct pollfd> poll_args;
    while (true)  {
        // 准备poll()的参数
        poll_args.clear();
        // 为了方便，把监听的文件描述符放在第一个位置
        struct pollfd pfd = {fd, POLLIN, 0};
        poll_args.push_back(pfd);
        // 连接的文件描述符
        for (Conn *conn : fd2conn)  {
            if (!conn)  {
                continue;
            }
            struct pollfd pfd = {};
            pfd.fd = conn->fd;
            pfd.events = (conn->state == STATE_REQ)? POLLIN : POLLOUT;
            pfd.events = pfd.events | POLLERR;
            poll_args.push_back(pfd);
        }

        // 轮询（poll）活跃的文件描述符
        // 这里的超时参数没啥用，随便设个大点儿的数就行
        int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), 1000);
        if ( rv < 0)  {
            die("poll");
        }

        // 处理活跃的连接
        for (size_t i = 1; i < poll_args.size(); ++i)  {
            if (poll_args[i].revents)  {
                Conn *conn = fd2conn[poll_args[i].fd];
                connection_io(conn);
                if (conn->state == STATE_END)  {
                    // 客户端正常关闭，或者出了啥问题
                    // 销毁这个连接
                    fd2conn[conn->fd] = NULL;
                    (void)close(conn->fd);
                    free(conn);
                }
            }
        }

        // 如果监听的文件描述符活跃，尝试接受一个新连接
        if (poll_args[0].revents)  {
            (void)accept_new_conn(fd2conn, fd);
        }
    }

}