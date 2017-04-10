#ifndef __MESSAGE_UTIL_H_
#define __MESSAGE_UTIL_H_
#include "umessage.h"

typedef struct {
    HeartbeatToMaster heartbeat_to_master;
} MessageHandle_t;
extern  MessageHandle_t g_MessageHandle;

class MessageUtil : public StateMachine {
public:
    MessageUtil();
    virtual ~MessageUtil();
    static unsigned Flowno();
    static int SendMessageToBEV(struct bufferevent *bev);
    UMessage& getUm() {
        return Um;
    }


protected:
    UMessage Um;
};

#endif

