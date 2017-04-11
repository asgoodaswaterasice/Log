#ifndef __GATE_HEARTBEAT_TO_MASTER_H_
#define __GATE_HEARTBEAT_TO_MASTER_H_

#include "manager_thread.h"
#include "my_uuid.h"
#include "message_util.h"
#include "umessage.h"
#include <iostream>
using namespace std;
using namespace ucloud;

void heartbeatInit(ManagerThread* manager);
void heartbeat_request_cb();
void heartbeat_response_cb(UMessage* pUm);
#endif 
