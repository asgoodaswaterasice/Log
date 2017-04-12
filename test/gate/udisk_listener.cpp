#include"udisk_listner.h"

void UDiskListener::Init() {
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
void UDiskListener::accept_cb(int fd, short events, void* arg) {
    UDiskListener *instance = (UDiskListener *) arg;
    int sockfd;
    struct sockaddr_un client;
    memset(&client, 0, sizeof(client));
    socklen_t len = 0;
    sockfd = ::accept(fd, (struct sockaddr *) &client, &len);
    cout << "the unix_socket client: " << client.sun_path << "connected, and the fd: " << sockfd << endl;
    evutil_make_socket_nonblocking(sockfd);
    UDiskIO *disk_io = new UDiskIO(); //每个udisk开启一个线程
    disk_io->Init();
    disk_io->qemuBev_ = bufferevent_socket_new(disk_io->base_, sockfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(disk_io->qemuBev_, UDiskIO::qemu_read_cb, NULL, UDiskIO::qemu_event_cb, (void *) disk_io);
    bufferevent_enable(disk_io->qemuBev_, EV_READ | EV_PERSIST);
    disk_io->Start(); //启动线程
}
