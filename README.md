# Redis inCpp

参考教程链接: https://cppguide.cn/pages/writemyredisfromscratch01/

运行server
```bash
./output/main server
```

server运行结果
```
Server listening on port 1234...
New connection: fd=5
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
zset_lookup: zset->tree is NULL
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
fd=-1160119776 read event
g_data.heap.size() = 1
fd=-1160119776 read event
find removing key
removing key: k
fd=-1160119776 read event
removing idle connection: 5
Connection closed
```


运行client
```bash
./output/main client
```

client运行结果
```
server says: [0] OK
> zadd set 1 q
server says: [0] OK
> zadd set 2 w
server says: [0] OK
> zadd set 3 e
server says: [0] OK
> zadd set 4 r
server says: [0] OK
> zquery set 1 q 2 4
received 38 bytes
server says: [4]
name_len: 1 name: e
score_len: 8 score: 3.000000
name_len: 1 name: r
score_len: 8 score: 4.000000
> zdel set q
server says: [1] Unknown cmd
> expire k 3000
server says: [2] Not found
> ttl set
server says: [2] -1
> zquery set 1 q 2 4
received 38 bytes
server says: [4]
name_len: 1 name: e
score_len: 8 score: 3.000000
name_len: 1 name: r
score_len: 8 score: 4.000000
> set k 3
server says: [0] OK
> expire k 2000
server says: [0] OK
> ttl k
server says: [0] 595
> ttl k
server says: [2] -2
```