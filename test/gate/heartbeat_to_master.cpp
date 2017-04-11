#include "heartbeat_to_master.h"

void heartbeatInit(ManagerThread* manager) {
    manager->getDispatcher().register_message_cb(ubs2::HEARTBEAT_CHUNK_TO_MASTER_RESPONSE, heartbeat_response_cb);
    manager->getTimer().register_timer_cb(ubs2::HEARTBEAT_CHUNK_TO_MASTER_REQUEST, heartbeat_request_cb);
}

void heartbeat_request_cb() {
    UMessage sReq;
    NewMessage_v2(&sReq, Flowno(), MyUuid::NewUuid(), ubs2::HEARTBEAT_CHUNK_TO_MASTER_REQUEST, 0,
            false, 0, 0, "heartbeat", NULL, NULL);
    ubs2::HeartbeatChunkToMasterRequest &sReqBody =
        *sReq.mutable_body()->MutableExtension(ubs2::heartbeat_chunk_to_master_request);
    sReqBody.set_host_id("1111111111111111111111");
    SendMessageToBEV(sReq, ManagerThread::GetInstance()->getMasterBev());

}

void heartbeat_response_cb(UMessage* pUm) {
    std::cout << "In heartbeat resonse callback" << std::endl;
    const ubs2::HeartbeatChunkToMasterResponse &sRes =
        pUm->body().GetExtension(ubs2::heartbeat_chunk_to_master_response);

    if (sRes.rc().retcode() != 0) {
        cout << "heartbeat response error: " << sRes.rc().error_message().c_str() << endl;
    }
}
