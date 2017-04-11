#ifndef  __GATE_MANAGER_THREAD_H_
#define __GATE_MANAGER_THREAD_H_

#include "message_plug.h"
#include "umessage.h"
#include<pthread.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<event.h>
#include<assert.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>
#include<event2/thread.h>
#include<errno.h>
#include<map>

using namespace ucloud;

typedef  void (*message_cb_t)(UMessage* pUm);
typedef  void (*timer_cb_t)();

class ProtobufDispatcher {
public:
    void register_message_cb(int message_type, message_cb_t cb);
    void dispatch(UMessage * pUm);

private:
    std::map<int, message_cb_t> mapMessgaeCb_;
};

class Timer {
public:
    explicit Timer(struct event_base* base) {
        struct timeval interval = {5, 0};
        struct event* ev;
        ev = event_new(base, -1, EV_TIMEOUT | EV_PERSIST, dispatch_timer_cb, this);
        event_add(ev, &interval);
    }
    void register_timer_cb(int message_type, timer_cb_t cb);
    void unregister_timer_cb(int message_type); // 超时后不再执行

    static void dispatch_timer_cb(int fd, short what, void* arg) {
        Timer* timer = (Timer*) arg;
        for (map<int, timer_cb_t>::iterator it = timer->mapTimerCb_.begin();
                it != timer->mapTimerCb_.end(); it++) {
            it->second();
        }

    }
private:
    std::map<int, timer_cb_t> mapTimerCb_;
};

class ManagerThread {
public:

    static ManagerThread* GetInstance();
    void Init();
    void Start();
    void connect2master();
    ProtobufDispatcher& getDispatcher() {
        return dispatcher_;
    }
    Timer& getTimer() {
        return timer_; 
    }
    struct bufferevent* getMasterBev() {
        return masterBev_;
    }

    static void protobuf_read_cb(bufferevent* bev, void* arg);
    static void master_event_cb(struct bufferevent *bev, short event, void *arg);

    static void* thread_func(void* arg);

private:

    ManagerThread();
    pthread_t  tid_;
    struct event_base* base_;
    struct bufferevent* masterBev_;
    ProtobufDispatcher dispatcher_;
    Timer timer_;
    static ManagerThread* pInstance_;
};
#endif
