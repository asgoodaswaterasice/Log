#ifndef __MESSAGE_UTIL_H_
#define __MESSAGE_UTIL_H_
#include "umessage.h"

unsigned Flowno();
int SendMessageToBEV(const ::google::protobuf::Message &sMessage, struct bufferevent *bev);

#endif

