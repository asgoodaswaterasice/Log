#include "manager_thread.h"


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

ManagerThread::ManagerThread() {
    base_ = event_base_new();
    /* 支持线程 */
    evthread_use_pthreads();
    plug_handle(); //初始化message处理插件  g_sMessagePtr
}
void ManagerThread::Init() {
    connect2master();
    connect2udatabase();
    createTimer();
}
void ManagerThread::Start() {
    if(pthread_create(&tid_, NULL, thread_func, this) < 0) {
        cout <<  "create manager thread error" << strerror(errno);
    }
}

void ManagerThread::connect2master() {
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12510);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    masterBev_ = bufferevent_socket_new(base_, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    bufferevent_setcb(masterBev_, protobuf_read_cb, NULL, master_event_cb, (void*)this);
    bufferevent_enable(masterBev_, EV_READ | EV_PERSIST);

    if (bufferevent_socket_connect(masterBev_,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
        bufferevent_free(masterBev_);
        cerr << "connect master server error" << endl;
    }
}
void ManagerThread::protobuf_read_cb(bufferevent* bev, void* arg) {
    UMessage* pUMessage = &UMessage(); //分配的是栈空间
    size_t readable;
    struct evbuffer *input = bufferevent_get_input(bev);
    readable = evbuffer_get_length(input); char data[readable]; evbuffer_copyout(input, data, readable);
    if (g_sMessagePtr.DecodeMessage(&pUMessage, data, readable) == 0) {
        return; //数据的长度不够
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
        dispatcher_.dispatch(pUMessage); // 按消息类型调用回调

    }

}
void ManagerThread::master_event_cb(struct bufferevent *bev, short event, void *arg) {
    managerthread *instance = (managerthread *) arg;
    if (event & bev_event_connected) {
        cout << "connect to master success" << endl;
    } else {
        cout << "master disconnected";
        instance->masterbev_ = null;
        bufferevent_free(bev); // 释放bev， 会关闭连接
        instance->connect2master();  //重新连接master
    }
}
void ManagerThread::udatabase_event_cb(struct bufferevent *bev, short event, void *arg) {
    ManagerThread *instance = (ManagerThread*) arg;
    if (event & bev_event_connected) {
        cout << "connect to udatabase success" << endl;
    } else {
        cout << "udatabase disconnected";
        instance->udatabaseBev_ = null;
        bufferevent_free(bev); // 释放bev， 会关闭连接
        instance->connect2udatabase();  //重新连接udatabase
    }
}

void connect2udatabase() {
    struct sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    udatabaseBev_ = bufferevent_socket_new(base_, -1, BEV_OPT_CLOSE_ON_FREE | BEV_OPT_THREADSAFE);
    bufferevent_setcb(udatabaseBev_, protobuf_read_cb, NULL, udatabase_event_cb, (void*)this);
    bufferevent_enable(masterBev_, EV_READ | EV_PERSIST);

    if (bufferevent_socket_connect(udatabaseBev_,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
        bufferevent_free(udatabaseBev_);
        cerr << "connect udatabase server error" << endl;
    }

}

static void* thead_func(void* arg) {
    ManagerThread* manager = (ManagerThread*)arg;
    manager->tid = pthread_self();
    event_base_dispatch(manager->base_);
    event_base_free(manager->base_);  //线程即将退出
}