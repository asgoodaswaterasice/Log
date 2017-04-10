#ifndef  __GATE_PROTOBUF_DISPATCHER_H__
#define __GATE_PROTOBUF_DISPATCHER_H__
#include<sys/socket.h>
#include<unistd.h>
#include<event.h>
#include<assert.h>
#include<event2/bufferevent.h>
#include<event2/buffer.h>
#include<event2/util.h>
#include<map>


extern ProtobufDispatcher g_ProtobufDispatcher;

