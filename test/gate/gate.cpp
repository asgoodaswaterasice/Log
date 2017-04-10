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

using namespace std;


class GateServer
{
    public:
        GateServer()
        {
        }
        void Init() {
            base_ = event_base_new();
            int res;
            int listener;
            listener = ::socket(AF_UNIX, SOCK_STREAM, 0);
            assert( listener != -1 );
            evutil_make_socket_nonblocking(listener);
            //允许多次绑定同一个地址。要用在socket和bind之间
            evutil_make_listen_socket_reuseable(listener);
            unlink("/home/yeheng/unix_socket");
            struct sockaddr_un un;
            memset(&un, 0, sizeof(un));
            un.sun_family = AF_UNIX;
            strncpy(un.sun_path, "/home/yeheng/unix_socket", sizeof(un.sun_path)-1);
            res = ::bind(listener, (struct sockaddr*)&un, sizeof(un));
            assert( res != -1 );
            res = ::listen(listener, 10);
            assert( res != -1 );

            //添加监听客户端请求连接事件
            struct event* ev_listen = event_new(base_, listener, EV_READ | EV_PERSIST, accept_cb, this);
            event_add(ev_listen, NULL);
        }
        void Start() {
            event_base_dispatch(base_);
            event_base_free(base_);
        }
    private:
        static void accept_cb(int fd, short events, void* arg) {
            GateServer* instance = (GateServer*)arg;
            int sockfd;
            struct sockaddr_un client;
            memset(&client, 0, sizeof(client));
            socklen_t len=0;
            sockfd = ::accept(fd, (struct sockaddr*)&client, &len);
            cout << "the unix_socket client: " << client.sun_path << "connected, and the fd: "  << sockfd << endl;      
            evutil_make_socket_nonblocking(sockfd);
            instance->qemuBev_ = bufferevent_socket_new(instance->base_, sockfd, BEV_OPT_CLOSE_ON_FREE);
            bufferevent_setcb(instance->qemuBev_, qemu_read_cb, NULL, qemu_event_cb, (void*)instance);
            bufferevent_enable(instance->qemuBev_, EV_READ | EV_PERSIST);
        }
        static void qemu_read_cb(bufferevent* bev, void* arg)
        {
            GateServer* instance = (GateServer*)arg;
            GateCmdProto::protohead head;
            struct evbuffer *input = bufferevent_get_input(bev);
            size_t readable = evbuffer_get_length(input);
            while (readable  >= sizeof(GateCmdProto::protohead)) { // head len enough
                evbuffer_copyout(input, &head, sizeof(head));
                if (readable >= head.size) { // data len enough
                    readable -= head.size;
                    uint32_t chunk_port = head.magic;
                    if(instance->chunkConnId2Bev_.find(chunk_port) == instance->chunkConnId2Bev_.end()) {
                        if(Connect2Chunk(chunk_port, instance) == -1) { //连接建立失败，丢弃数据上报故障
                            evbuffer_drain(input, head.size);
                        } else {
                            evbuffer_remove_buffer(input, bufferevent_get_output(instance->chunkConnId2Bev_[chunk_port]), head.size); //连接没有建立也可以向输出缓冲区发送数据
                        }
                    } else {
                        evbuffer_remove_buffer(input, bufferevent_get_output(instance->chunkConnId2Bev_[chunk_port]), head.size); //连接没有建立也可以向输出缓冲区发送数据
                    }
                } else {
                    break;
                }
            }
        }

        static int Connect2Udatabase(GateServer* instance) {
            struct sockaddr_in udb_addr;
            memset(&udb_addr,0,sizeof(udb_addr));
            udb_addr.sin_family = AF_INET;
            udb_addr.sin_port = htons(888);
            udb_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            udbBev_ = bufferevent_socket_new(instance->base_, -1, BEV_OPT_CLOSE_ON_FREE);
            bufferevent_setcb(udbBev_, udb_read_cb, NULL, udb_event_cb, (void*)instance);
            bufferevent_enable(chunk_bev, EV_READ | EV_WRITE | EV_PERSIST);



        
        }

        static int Connect2Chunk(uint32_t connId, GateServer* instance) {
            struct sockaddr_in server_addr;
            memset(&server_addr,0,sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(connId);
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            struct bufferevent* chunk_bev = bufferevent_socket_new(instance->base_, -1, BEV_OPT_CLOSE_ON_FREE);
            bufferevent_setcb(chunk_bev, chunk_read_cb, NULL, chunk_event_cb, (void*)instance);
            bufferevent_enable(chunk_bev, EV_READ | EV_WRITE | EV_PERSIST);

            if (bufferevent_socket_connect(chunk_bev,(struct sockaddr*)&server_addr,sizeof(server_addr)) < 0) {
                bufferevent_free(chunk_bev);
                cerr << "connect chunk server error, port: " << connId << endl;
                return -1;
            }
            instance->chunkConnId2Bev_[connId] = chunk_bev;
            instance->chunkBev2ConnId_[chunk_bev] = connId;
            return 0;
        }
        static void qemu_event_cb(struct bufferevent *bev, short event, void *arg) {
            GateServer* instance = (GateServer*)arg;
            if (event & BEV_EVENT_CONNECTED) {
                cout << "connect to gate success" << endl;
            } else {  
                cout << "qemu disconnected";
                instance->chunkConnId2Bev_.clear();
                instance->chunkBev2ConnId_.clear();
                bufferevent_free(bev); // 释放bev， 会关闭连接
            }

        }


        static void chunk_read_cb(struct bufferevent* bev, void* arg) {
            GateServer* instance = (GateServer*)arg;
            GateCmdProto::protohead head;
            struct evbuffer *input = bufferevent_get_input(bev);
            size_t readable = evbuffer_get_length(input);
            while (readable  >= sizeof(GateCmdProto::protohead)) { // head len enough
                evbuffer_copyout(input,&head,sizeof(head));
                if (readable >= head.size) { // data len enough
                    readable -= head.size;
                    if(instance->qemuBev_ != NULL) {
                        evbuffer_remove_buffer(input, bufferevent_get_output(instance->qemuBev_), head.size);//将数据发回qemu
                    } else {
                        evbuffer_drain(input, head.size);  // qemu到gate的连接断开，丢弃chunk返回的数据
                    }
                } else {
                    break;
                }
            }
        }
        static void chunk_event_cb(struct bufferevent *bev, short events, void *arg) {
            GateServer* instance = (GateServer*) arg;
            if (events & BEV_EVENT_CONNECTED) {
                cout << "connect to chunk success" << endl;
            } else {  
                assert(instance->chunkBev2ConnId_.find(bev) != instance->chunkBev2ConnId_.end());
                assert(instance->chunkConnId2Bev_.find(instance->chunkBev2ConnId_[bev]) == instance->chunkConnId2Bev_.end());
                instance->chunkConnId2Bev_.erase(instance->chunkBev2ConnId_[bev]);
                bufferevent_free(bev); //只要有错误，就断开连接等待后面的重连
                instance->chunkBev2ConnId_.erase(bev);
            }

        }

        struct event_base* base_;
        struct bufferevent* qemuBev_;
        struct bufferevent* udatabaseBev_;
        std::map<uint32_t, struct bufferevent*> chunkConnId2Bev_; //port 到 chunk connection bufferevent 的map
        std::map<struct bufferevent*, uint32_t> chunkBev2ConnId_; //chunk connection bufferevent 到 port 的map

};


int main(int argc, char** argv)
{
    
    GateServer server;
    server.Init();
    server.Start();
    return 0;
}





