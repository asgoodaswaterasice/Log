#ifndef  __GATE_MANAGER_THREAD_H__
#define __GATE_MANAGER_THREAD_H__

#include "message_plug.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<event.h>
#include<assert.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>
#include<map>

using namespace ucloud;

typedef  void (*message_cb_t)(UMessage* pUm);
typedef  void (*timer_cb_t)();

class ProtobufDispatcher {
public:
    void register_message_cb(int message_type, message_cb_t cb);
    void dispatch(UMessage * pUm);

private:
    map<int, message_cb_t> mapMessgaeCb_;
};

class Timer {
    Timer(struct event_base* base) {
        struct timeval interval = {5, 0};
        struct event* ev;
        ev = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, timer_cb, this);
        event_add(ev, interval);
    }
    void register_timer_cb(int message_type, timer_cb_t cb);
    void unregister_timer_cb(int message_type); // 超时后不再执行

    static void dispatch_timer_cb(int fd, short what, void* arg) {
        for (map<int, message_cb_t>::iterator it = mapMessgaeCb_.begin();
                it != mapMessgaeCb_.end(); it++) {
            it->second();
        }

    }
private:
    static map<int, message_cb_t> mapMessgaeCb_;


};

class ManagerThread {
public:

    ManagerThread();
    void Init();
    void Start();
    void connect2master();
    static void protobuf_read_cb(bufferevent* bev, void* arg);
    static void master_event_cb(struct bufferevent *bev, short event, void *arg);
    static void udatabase_event_cb(struct bufferevent *bev, short event, void *arg);

    void connect2udatabase();

    static void* thead_func(void* arg);
    friend  ProtobufDispatcher;
    friend  Timer;

private:

    pthread_t  tid_;
    struct event_base* base_;
    struct bufferevent* masterBev_;
    struct bufferevent* udatabaseBev_;
    static ProtobufDispatcher dispatcher_;
    static Timer timer_;
};
#endif