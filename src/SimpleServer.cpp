#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SimpleServer.h"
#include "util.h"

static void SimpleServer_do_something(int connfd)  {
    char rbuf[64]  =  {};
    ssize_t n =  read(connfd,  rbuf,  sizeof(rbuf)  -  1);
    if  (n <  0)  {
        msg("read() error");
        return ;
    }
    printf("client says: %s\n",   rbuf);

    char wbuf[]  =  "world";
    write(connfd,  wbuf,  strlen(wbuf));
}

// const size_t k_max_msg =  4096;
// static int errno;

static int32_t SimpleServer_one_request(int connfd)  {
    //  4字节的头部
    char rbuf[4 +  k_max_msg +  1];
    errno =  0;
    int32_t err =  read_full(connfd,  rbuf,  4);
    if  (err)  {
        if  (errno ==  0)  {
            msg("EOF");
        }  else  {
            msg("read() error");
        }
        return  err;
    }

    uint32_t len =  0;
    memcpy(&len,  rbuf,  4);    // 假设是小端序
    if  (len >  k_max_msg)  {
        msg("too long");
        return  -1;
    }

    // 请求体
    err =  read_full(connfd,  &rbuf[4],  len);
    if  (err)  {
        msg("read() error");
        return  err;
    }

    // 做点什么
    rbuf[4 +  len]  =   '\0';
    printf("client says: %s\n",  &rbuf[4]);

    // 用同样的协议回复
    const char reply[]  =  "world";
    char wbuf[4 +  sizeof(reply)];
    len =  (uint32_t)strlen(reply);

    memcpy(wbuf,  &len,  4);
    memcpy(&wbuf[4],  reply,  len);
    return  write_all(connfd,  wbuf,  4 +  len);
}



void SimpleServer(){
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

    while  (true)  {
        //  accept
        struct  sockaddr_in client_addr =  {};
        socklen_t socklen =  sizeof(client_addr);
        int connfd =  accept(fd,   (struct  sockaddr * )&client_addr,  &socklen);
        if  (connfd <  0)  {
            continue;      // 出错了，跳过这次循环
        }

        // SimpleServer_do_something(connfd);
      // 一次只处理一个客户端连接
        while  (true)  {
            int32_t err =  SimpleServer_one_request(connfd);
            if  (err)  {
                break;
            }
        }
        close(connfd);
    }

}

