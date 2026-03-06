#include "SimpleClient.h"

const size_t k_max_msg =  4096;
static int errno;
static int32_t query(int fd,  const char *text)  {
    uint32_t len =  (uint32_t)strlen(text);
    if  (len >  k_max_msg)  {
        return  -1;
    }

    char wbuf[4 +  k_max_msg];
    memcpy(wbuf,  &len,  4);    // 假设是小端序
    memcpy(&wbuf[4],  text,  len);
    if  (int32_t err =  write_all(fd,  wbuf,  4 +  len))  {
        return  err;
    }

    //  4字节的头部
    char rbuf[4 +  k_max_msg +  1];
    errno =  0;
    int32_t err =  read_full(fd,  rbuf,  4);
    if  (err)  {
        if  (errno ==  0)  {
            msg("EOF");
        }  else  {
            msg("read() error");
        }
        return  err;
    }

    memcpy(&len,  rbuf,  4);    // 假设是小端序
    if  (len >  k_max_msg)  {
        msg("too long");
        return  -1;
    }

    // 回复体
    err =  read_full(fd,  &rbuf[4],  len);
    if  (err)  {
        msg("read() error");
        return  err;
    }

    // 做点什么
    rbuf[4 +  len]  =   '\0';
    printf("server says: %s\n",  &rbuf[4]);
    return  0;
}


void SimpleClient(){
    int fd =  socket(AF_INET,  SOCK_STREAM,  0);
    if  (fd <  0)  {
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

    // char msg[]  =  "hello";
    // write(fd,  msg,  strlen(msg));

    // char rbuf[64]  =  {};
    // ssize_t n =  read(fd,  rbuf,  sizeof(rbuf)  -  1);
    // if  (n <  0)  {
    //     die("read");
    // }
    // printf("server says: %s\n",   rbuf);

    // 多个请求
    int32_t err =  query(fd,   "hello1");
    if  (err)  {
        printf("query error\n");
        goto  L_DONE;
    }
    err =  query(fd,  "hello2");
    if  (err)  {
        goto  L_DONE;
    }
    err =  query(fd,  "hello3");
    if  (err)  {
        goto  L_DONE;
    }

L_DONE:
    close(fd);
    // return  0;

}