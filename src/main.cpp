#include <stdio.h>
#include "SimpleServer.h"
#include "SimpleClient.h"
#include "PollServer.h"
#include "RedisServer.h"
#include "RedisClient.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <key> [value]\n", argv[0]);
        return 1;
    }
    const char* key = argv[1];
    if (strcmp(key, "server") == 0) {      // ✅ 正确比较字符串
        // SimpleServer();
        // PollServer();
        RedisServer();
    } else if (strcmp(key, "client") == 0) {
        // SimpleClient();
        RedisClient();
    }  else {
        printf("Unknown command: %s\n", key);
        return 1;
    }
    return 0;
}