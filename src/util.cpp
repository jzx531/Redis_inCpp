#include "util.h"

// 辅助函数（根据你的代码推测）
void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}

int32_t read_full(int fd,  char *buf,  size_t n)  {
    while  (n >  0)  {
        ssize_t rv =  read(fd,  buf,  n);
        if  ( rv <=  0)  {
            return  -1;    // 出错了，或者遇到意外的文件结束符（EOF）
        }
        assert((size_t)rv <=  n);
        n -=  (size_t)rv;
        buf +=  rv;
    }
    return  0;
}

int32_t write_all(int fd,  const char *buf,  size_t n)  {
    while  (n >  0)  {
        ssize_t rv =  write(fd,  buf,  n);
        if  ( rv <=  0)  {
            return  -1;   // 出错了
        }
        assert((size_t)rv <=  n);
        n -=  (size_t)rv;
        buf +=  rv;
    }
    return  0;
}

// 把fd设置为非阻塞模式的系统调用是fcntl
void fd_set_nb(int fd)  {
    errno =  0;
    int flags =  fcntl(fd,  F_GETFL,  0);
    if  (errno)  {
        die("fcntl error");
        return ;
    }

    flags | =  O_NONBLOCK;

    errno =  0;
    (void)fcntl(fd,  F_SETFL,  flags);
    if  (errno)  {
        die("fcntl error");
    }
}
