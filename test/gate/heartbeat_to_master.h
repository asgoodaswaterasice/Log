/*
 * heartbeat_to_master.h
 *
 *  Created on: Mar 24, 2012
 *      Author: Sennajox <sennajox at gmail dot com>
 */

#ifndef __GATE_HEARTBEAT_TO_MASTER_H_
#define __GATE_HEARTBEAT_TO_MASTER_H_

#include "protobuf_dispatcher.h"
#include "my_uuid.h"
#include <iostream>
using namespace ucloud;

class HeartbeatToMaster: public MessageUtil {
public:
    HeartbeatToMaster() {
        manager.get_timer().register_routine();
    }

    HeartbeatToMaster(int resType, uint32_t timeout){
      manager.get_dispatcher().register_message_cb(resType, heartbeat_response_cb);

    }
    static void heartbeat_request_cb() {
        UMessage um;
        NewMessage_v2(&um, Flowno(),MyUuid::NewUuid(), ubs2::HEARTBEAT_TO_MASTER_REQUEST, 0,
                      false, 0, 0, "heartbeat", NULL, NULL);
        SendMessageToBEV(manager.get_masterBev());

    }
    static void heartbeat_response_cb(UMessage* pUm) {
        std::cout << "In heartbeat resonse callback" << std::endl;
    }
};

#endif 
