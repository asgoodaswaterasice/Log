#ifndef __GATE_H_
#define __GATE_H_

#include "proto.h"
#include <iostream>
#include<sys/socket.h>
#include<sys/un.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<event.h>
#include<assert.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>
#include<map>
#include<pthread.h>
#include<errno.h>
#include "manager_thread.h"
#include "heartbeat_to_master.h"

class UDiskIO
{
public:
    UDiskIO();
    void Init();
    void Start();
private:
    static void accept_cb(int fd, short events, void* arg);
    static void qemu_read_cb(bufferevent* bev, void* arg);
    static int Connect2Udatabase(UDiskIO* instance);
    static void udb_read_cb(struct bufferevent* bev, void* arg);
    static void udb_event_cb(struct bufferevent *bev, short event, void *arg);
    static int Connect2Chunk(uint32_t connId, UDiskIO* instance);
    static void qemu_event_cb(struct bufferevent *bev, short event, void *arg);
    static void chunk_read_cb(struct bufferevent* bev, void* arg);
    static void chunk_event_cb(struct bufferevent *bev, short events, void *arg);

    static  void* thread_func(void* arg);
    pthread_t tid_;
    struct event_base* base_;
    struct bufferevent* qemuBev_;
    struct bufferevent* udbBev_;
    std::map<uint32_t, struct bufferevent*> chunkConnId2Bev_; //port 到 chunk connection bufferevent 的map
    std::map<struct bufferevent*, uint32_t> chunkBev2ConnId_; //chunk connection bufferevent 到 port 的map

};
#endif
