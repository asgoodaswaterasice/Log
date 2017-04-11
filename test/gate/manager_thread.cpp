#include "manager_thread.h"
#include <iostream>

using namespace std;


void ProtobufDispatcher::register_message_cb(int message_type, message_cb_t cb) {
    if (mapMessgaeCb_.find(message_type) == mapMessgaeCb_.end()) {
        mapMessgaeCb_[message_type] = cb;
    }
}

void ProtobufDispatcher::dispatch(UMessage *pUm) {
    /* if(isresponse) {
         if(istimeout) {
             // 丢弃该response
         } else {
             //清除定时，触发回调
         }
     } else {  //是request
        // 触发回调
     } */
    int type = pUm->head().message_type();
    cout << "respose type:" << type << endl;
    if (mapMessgaeCb_.find(type) == mapMessgaeCb_.end()) {
        cout << "No callback for the message type: " << type << endl;
    } else {
        mapMessgaeCb_[type](pUm);
    }

}

void Timer::register_timer_cb(int message_type, timer_cb_t cb) {
    if (mapTimerCb_.find(message_type) == mapTimerCb_.end()) {
        mapTimerCb_[message_type] = cb;
    }
}



ManagerThread* ManagerThread::GetInstance() { // 饿汉单例,线程安全
    static ManagerThread sInstance;
    return &sInstance;
}

ManagerThread::ManagerThread(): base_(event_base_new()), timer_(base_) {
   // base_ = event_base_new();
    /* 支持线程 */
    evthread_use_pthreads();
    plug_handle(); //初始化message处理插件  g_sMessagePtr
    cout << "base: " << (long)base_ << "timer" << (long)&timer_ << endl;
}
void ManagerThread::Init() {
    connect2master();
}
void ManagerThread::Start() {
    if(pthread_create(&tid_, NULL, thread_func, this) < 0) {
        cout <<  "create manager thread error" << strerror(errno) << endl;
    }
}

void ManagerThread::connect2master() {
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12510);
    server_addr.sin_addr.s_addr = inet_addr("192.168.150.35");
    masterBev_ = bufferevent_socket_new(base_, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    bufferevent_setcb(masterBev_, protobuf_read_cb, NULL, master_event_cb, (void*)this);
    bufferevent_enable(masterBev_, EV_READ | EV_PERSIST);

    if (bufferevent_socket_connect(masterBev_,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
        bufferevent_free(masterBev_);
        cerr << "connect master server error" << endl;
    }
}
void ManagerThread::protobuf_read_cb(bufferevent* bev, void* arg) {
    ManagerThread* instance = (ManagerThread*) arg;
    UMessage um;
    void* pUMessage = &um; //分配的是栈空间
    size_t readable;
    struct evbuffer *input = bufferevent_get_input(bev);
    readable = evbuffer_get_length(input);
    char* data = new char[readable]; 
    evbuffer_copyout(input, data, readable);
    int res = g_sMessagePtr.DecodeMessage(&pUMessage, data, readable);
    delete [] data;
    if (res == -1) {
        cout << "Decode message error" << endl;
    } else if(res == 0) {
        cout << "Message length not enough" << endl;
        return;
    } else {
        /* if(isresponse) {
             if(istimeout) {
                 // 丢弃该response
             } else {
                 //清除定时，触发回调
             }
         } else {  //是request
            // 触发回调
         } */
        instance->dispatcher_.dispatch((UMessage*)pUMessage); // 按消息类型调用回调
        evbuffer_drain(input, readable);

    }

}
void ManagerThread::master_event_cb(struct bufferevent *bev, short event, void *arg) {
    ManagerThread *instance = (ManagerThread *) arg;
    if (event & BEV_EVENT_CONNECTED) {
        cout << "connect to master success" << endl;
    } else {
        cout << "master disconnected" << endl;
        instance->masterBev_ = NULL;
        bufferevent_free(bev); // 释放bev， 会关闭连接
        instance->connect2master();  //重新连接master, 待优化，防止断开后疯狂重连
    }
}

void* ManagerThread::thread_func(void* arg) {
    ManagerThread* manager = (ManagerThread*)arg;
    manager->tid_ = pthread_self();
    event_base_dispatch(manager->base_);
    event_base_free(manager->base_);  //线程即将退出
    return (void*) 0;
}
