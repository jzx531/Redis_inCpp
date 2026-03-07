#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "SimpleServer.h"
#include <errno.h>     // errno, EAGAIN, EINTR
#include <fcntl.h>

// 辅助函数（根据你的代码推测）
void die(const char *msg) ;

void msg(const char *msg) ;

int32_t read_full(int fd,  char *buf,  size_t n);

int32_t write_all(int fd,  const char *buf,  size_t n);

void fd_set_nb(int fd);

#endif

