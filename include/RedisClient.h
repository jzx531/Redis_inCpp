#ifndef REDIS_CLIENT_H
#define REDIS_CLIENT_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "util.h"

void RedisClient();

#endif // REDIS_CLIENT_H
