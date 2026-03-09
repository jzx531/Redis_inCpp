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
#include <time.h>

const size_t k_max_msg =  4096;
const size_t k_max_args = 10;

// 辅助函数（根据你的代码推测）
void die(const char *msg) ;

void msg(const char *msg) ;

int32_t read_full(int fd,  char *buf,  size_t n);

int32_t write_all(int fd,  const char *buf,  size_t n);

void fd_set_nb(int fd);

//定时器函数
uint64_t get_monotonic_usec();

const uint64_t k_idle_timeout_ms =  50 *   1000;

uint32_t next_timer_ms();

void process_timers();

#endif

