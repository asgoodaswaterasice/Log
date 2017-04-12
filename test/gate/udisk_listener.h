#ifndef __GATE_UDISK_LISTENER_H_
#define  __GATE_UDISK_LISTENER_H_

class UDiskListener {

public:
    void Init();
    static  accept_cb(int fd, short events, void* arg);

};
#endif
