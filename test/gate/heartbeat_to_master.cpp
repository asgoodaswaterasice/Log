/*
 * heartbeat_to_master.cpp
 *
 *  Created on: Mar 24, 2012
 *      Author: Sennajox <sennajox at gmail dot com>
 */

#include "ubs_proxy.h"
#include "umessage_ptr.h"
#include "my_uuid.h"
#include "heartbeat_to_master.h"
#include "migrate_ubs.h"

using namespace std;
using namespace ucloud;
using namespace ucloud::ubs2common;
using namespace ucloud::ubs2proxy;

static const int MYID = SMI_HEARTBEAT_TO_MASTER;

FACTORY_IMPLEMENT(HeartbeatToMasterMachine, MYID);

const int CYCLE_TIME = 2;

REGISTER_ROUTING_MACHINE(HeartbeatToMasterMachine, CYCLE_TIME, false);

DEFAULT_IMPLEMENT2(MYID, INIT_STATE, HeartbeatToMasterMachine, Entry_Init);
DEFAULT_IMPLEMENT2(MYID, WAIT_RESPONSE_STATE, HeartbeatToMasterMachine, Entry_WaitResponse);
DEFAULT_IMPLEMENT2(MYID, LOAD_TASK_STATE, HeartbeatToMasterMachine, Entry_LoadTask);

HeartbeatToMasterMachine::HeartbeatToMasterMachine()
{
    // 该主动状态机不要记录协议日志
    m_bRecordMessage = false;
}

HeartbeatToMasterMachine::~HeartbeatToMasterMachine()
{
}

void HeartbeatToMasterMachine::Dump(int iFD)
{
}

int HeartbeatToMasterMachine::Timeout(bool bMachineTimeout)
{
    ERROR("%s", "Timeout occurred, delete myself");
    return SM_FINISH;
}

int HeartbeatToMasterMachine::StateMachineType()
{
    return MYID;
}

int HeartbeatToMasterMachine::Entry_Init(conn_key_t key, void *pMessage)
{
    assert(!pMessage);

    if (false == g_bReadyFlag) {
        DEBUG("Can't get any hashkey, ip: %s" ,g_strIp.c_str());
        return SM_ERROR;
    }

    UMessage sReq;
    NewMessage(&sReq, Flowno(), MyUuid::NewUuid(),
            ubs2::HEARTBEAT_PROXY_TO_MASTER_REQUEST, WORKERINDEX, false,
            m_iID, 0, NULL, NULL, NULL);
    ubs2::HeartbeatProxyToMasterRequest &sReqBody =
            *sReq.mutable_body()->MutableExtension(ubs2::heartbeat_proxy_to_master_request);

    sReqBody.set_proxy_id(g_iProxyID);
    sReqBody.set_ip(g_strIp);
    sReqBody.set_port(g_iPort);
    // 增加iscsi/gate_port用以支持live migrate, 增加utm_port用以支持migrate后rebase
    sReqBody.set_iscsi_port(g_iIscsiPort);
    sReqBody.set_gate_port(g_iGatePort);
    sReqBody.set_utm_port(g_iUTMPort);
    // FIXME: 如果上一次漏掉了同一秒的怎么办?!
    sReqBody.set_max_index_time(g_iMaxIndexTime+1);

    // 仅上报仍活跃的IQN
    bool migrating = false;
    for(SessionMap::const_iterator it = g_mapSessions.begin();
            it != g_mapSessions.end(); ++it) {
        ucloud::ubs2proxy::Session* pSession = it->second;
        if (false == pSession->IsAlive())
            continue;
        ubs2::ActiveIqn* pActiveIqn = sReqBody.add_active_iqns();
        pActiveIqn->set_iqn(pSession->dev().iqn());
        pActiveIqn->set_ubs_id(pSession->dev().ubs_id());
        pActiveIqn->set_snapshot_id("");
        pActiveIqn->set_last_active_time(pSession->LastActiveTime());

        ubs2::IOStats* ioStats = pActiveIqn->mutable_io_stats();
        ioStats->set_iops_read( 1.0*pSession->GetAmountIoRead()/CYCLE_TIME );
        ioStats->set_iops_write( 1.0*pSession->GetAmountIoWrite()/CYCLE_TIME );
        ioStats->set_byteps_read( 1.0*pSession->GetAmountByteRead()/CYCLE_TIME );
        ioStats->set_byteps_write( 1.0*pSession->GetAmountByteWrite()/CYCLE_TIME );
        ioStats->set_stats_time( g_sWIWOInterface.time_now()->tv_sec );

        // XXX:不管是否心跳成功，都应将统计初始化，因为否则会造成单位不一致
        pSession->ResetStats();
        if (pSession->migrate_ubs()) migrating = true;
    }

    if (migrating) sReqBody.set_migrating_ubs(true);

    __REQ2SERVER_LEADER_INROUTINE(MASTER_FULL_NAME, sReq);

    m_iState = WAIT_RESPONSE_STATE;
    return SM_CONTINUE;
}

int HeartbeatToMasterMachine::Entry_WaitResponse(conn_key_t key, void *pMessage)
{
    assert(pMessage);
    umessage_ptr<UMessage> pResponse((UMessage*) pMessage);

    assert(pResponse->head().message_type() == ubs2::HEARTBEAT_PROXY_TO_MASTER_RESPONSE);
    assert(pResponse->body().HasExtension(ubs2::heartbeat_proxy_to_master_response));
    const ubs2::HeartbeatProxyToMasterResponse &sRes =
            pResponse->body().GetExtension(ubs2::heartbeat_proxy_to_master_response);
    if (sRes.rc().retcode() != 0) {
        ERROR("%s", sRes.rc().error_message().c_str());
    }

    for (int32_t i = 0; i < sRes.indexes_size(); i++) {
        const ubs2::Index& sIndex = sRes.indexes(i);
        INFO("Get new index :%s", sIndex.id().c_str());
        // 先清理旧的，然后更新为新的
        g_sIndexMgr.DelIndex(sIndex.id());
        g_sIndexMgr.AddIndex(sIndex);
        // 记录当前最大的Index时间
        if (g_iMaxIndexTime < sIndex.modify_time()) {
            g_iMaxIndexTime = sIndex.modify_time();
        }
    }
    
    for (int32_t i = 0; i < sRes.iqn_indexes_size(); ++i) {
        std::map<std::string, ucloud::ubs2proxy::Device*>::iterator itDev = g_sMountedDevices.find(sRes.iqn_indexes(i).iqn());
        // 防止在更新过程中，lc已经卸载掉了
        if (itDev == g_sMountedDevices.end()) {
            DEBUG("Target ubs is not login:%s", sRes.iqn_indexes(i).iqn().c_str());
            return SM_FINISH;
        }

        /*
        // 根据父子关系构建索引层次信息
        std::string strCurIndexId = sRes.iqn_indexes(i).index_id();
        int iRet = g_sIndexMgr.BuildIndexLayers(itDev->second->iqn(), strCurIndexId);
        if (0 != iRet) {
            ERROR("Fail to build index layers, iqn:%s, ret:%d", 
                    itDev->second->iqn().c_str(), iRet);
            return SM_ERROR;
        }
        */
    }

    for (int i = 0; i < sRes.tuple_size(); i += 3) {
        g_sIndexMgr.InsertTuple(sRes.tuple(i), sRes.tuple(i+1), sRes.tuple(i+2));
    }

    if (g_sIndexMgr.HasNonReadyIndex()) {
        // set all task not applied in case old task not appled
        SyncTaskMap::iterator itTask;
        for (itTask = g_sTaskMap.begin(); itTask!=g_sTaskMap.end(); itTask++) {
            itTask->second->set_applied(false);
        }
        return load_task();
    }

    return SM_FINISH;
}

int HeartbeatToMasterMachine::load_task()
{
    INFO("%s", "Load task");
    UMessage sReq;
    NewMessage_v2(&sReq, Flowno(), MyUuid::NewUuid(), ubs2::LOAD_TASK_REQUEST, WORKERINDEX, 
        false, m_iID, 0, NULL, NULL, NULL);
        
    sReq.mutable_body()->MutableExtension(ubs2::load_task_request);
    SendInternalRequest(sReq);
    m_iState = LOAD_TASK_STATE;
    return SM_CONTINUE;
}

int HeartbeatToMasterMachine::Entry_LoadTask(conn_key_t key, void *pMessage)
{
    umessage_ptr<UMessage> pRequest((UMessage *)pMessage);
    assert(pRequest->head().message_type() == ubs2::LOAD_TASK_RESPONSE);
    assert(pRequest->body().HasExtension(ubs2::load_task_response));

    const ubs2::LoadTaskResponse &sRes = pRequest->body().GetExtension(ubs2::load_task_response);

    if ( sRes.rc().retcode() != 0 ) {
        INFO("%s", "Load task fail, reload");
        return SM_FINISH;
    }

    INFO("%s", "Load task success, ready all index (task)");
    g_sIndexMgr.ReadyAllIndex();
    return SM_FINISH;
}

