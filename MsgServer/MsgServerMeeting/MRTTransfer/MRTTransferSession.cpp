//
//  MRTTransferSession.cpp
//  dyncRTMeeting
//
//  Created by hp on 11/26/15.
//  Copyright (c) 2015 hp. All rights reserved.
//

#include "MRTTransferSession.h"
#include "RTMessage.h"
#include "MRTRoomManager.h"
#include "MRTConnManager.h"
#include "MRTHttpSvrConn.h"
#include "RTUtils.hpp"

#define TIMEOUT_TS (60*1000)

MRTTransferSession::MRTTransferSession(TRANSFERMODULE module)
: RTTcp()
, RTJSBuffer()
, RTTransfer()
, m_lastUpdateTime(0)
, m_moduleId("")
, m_transferSessId("")
, m_module(module)
, m_addr("")
, m_port(0)
, m_connectingStatus(0)
{
    AddObserver(this);
}

MRTTransferSession::~MRTTransferSession()
{
    DelObserver(this);
    Unit();
}

void MRTTransferSession::Init()
{
    TCPSocket* socket = this->GetSocket();

    socket->Open();

    socket->InitNonBlocking(socket->GetSocketFD());
    socket->NoDelay();
    socket->KeepAlive();
    socket->SetSocketBufSize(96L * 1024L);

    this->SetTimer(120*1000);
}

void MRTTransferSession::Unit()
{
    Disconn();
    m_connectingStatus = 0;
}

bool MRTTransferSession::Connect(const std::string addr, int port)
{
    m_addr = addr;
    m_port = port;
    LI("MRTTransferSession::Connect(params) m_addr:%s, m_port:%d\n", m_addr.c_str(), m_port);
    OS_Error err = GetSocket()->Connect(SocketUtils::ConvertStringToAddr(m_addr.c_str()), m_port);
    if (err == OS_NoErr || err == EISCONN) {
        m_connectingStatus = 1;
        return true;
    } else {
        LE("%s ERR:%d\n", __FUNCTION__, err);
        return false;
    }
}

bool MRTTransferSession::Connect()
{
    if (m_addr.empty() || m_port < 2048) {
        LE("%s invalid params addr:%s, port:%d\n", __FUNCTION__, m_addr.c_str(), m_port);
        return false;
    }
    OS_Error err = GetSocket()->Connect(SocketUtils::ConvertStringToAddr(m_addr.c_str()), m_port);
    if (err == OS_NoErr || err == EISCONN) {
        m_connectingStatus = 1;
        return true;
    } else {
        LE("%s ERR:%d\n", __FUNCTION__, err);
        return false;
    }
}

void MRTTransferSession::Disconn()
{
    GetSocket()->Cleanup();
}

bool MRTTransferSession::RefreshTime()
{
    UInt64 now = OS::Milliseconds();
    if (m_lastUpdateTime <= now) {
        m_lastUpdateTime = now  + TIMEOUT_TS;
        RTTcp::UpdateTimer();
        return true;
    } else {
        return false;
    }
}

void MRTTransferSession::KeepAlive()
{
    TRANSFERMSG t_msg;
    CONNMSG c_msg;
    t_msg._action = TRANSFERACTION::req;
    t_msg._fmodule = TRANSFERMODULE::mmeeting;
    t_msg._type = TRANSFERTYPE::conn;
    t_msg._trans_seq = GenericTransSeq();
    t_msg._trans_seq_ack = 0;
    t_msg._valid = 1;

    c_msg._tag = CONNTAG::co_keepalive;
    c_msg._msg = "1";
    c_msg._id = "";
    c_msg._msgid = "";
    c_msg._moduleid = "";
    t_msg._content = c_msg.ToJson();

    std::string s = t_msg.ToJson();
    SendTransferData(s.c_str(), (int)s.length());
}


void MRTTransferSession::TestConnection()
{

}

void MRTTransferSession::EstablishConnection()
{
    TRANSFERMSG t_msg;
    CONNMSG c_msg;
    t_msg._action = TRANSFERACTION::req;
    t_msg._fmodule = TRANSFERMODULE::mmeeting;
    t_msg._type = TRANSFERTYPE::conn;
    t_msg._trans_seq = GenericTransSeq();
    t_msg._trans_seq_ack = 0;
    t_msg._valid = 1;

    c_msg._tag = CONNTAG::co_msg;
    c_msg._msg = "hello";
    c_msg._id = "";
    c_msg._msgid = "";
    c_msg._moduleid = "";

    t_msg._content = c_msg.ToJson();

    std::string s = t_msg.ToJson();
    SendTransferData(s.c_str(), (int)s.length());
}

void MRTTransferSession::SendTransferData(const char* pData, int nLen)
{
    RTTcp::SendTransferData(pData, nLen);
    GetSocket()->RequestEvent(EV_RE);
}

// from RTTcp
void MRTTransferSession::OnRecvData(const char*pData, int nLen)
{
    if (!pData) {
        return;
    }
    RTJSBuffer::RecvData(pData, nLen);
}

void MRTTransferSession::OnRecvMessage(const char*message, int nLen)
{
    RTTransfer::DoProcessData(message, nLen);
}

// from RTTransfer

void MRTTransferSession::OnTransfer(const std::string& str)
{
    RTTcp::SendTransferData(str.c_str(), (int)str.length());
}

void MRTTransferSession::OnMsgAck(TRANSFERMSG& tmsg)
{
    TRANSFERMSG ack_msg;
    if (tmsg._action == TRANSFERACTION::req) {
        ack_msg._action = TRANSFERACTION::req_ack;
    } else {
        ack_msg._action = TRANSFERACTION::resp_ack;
    }
    ack_msg._fmodule = TRANSFERMODULE::mmeeting;
    ack_msg._type   = tmsg._type;
    ack_msg._trans_seq = tmsg._trans_seq;
    ack_msg._trans_seq_ack = tmsg._trans_seq + 1;
    ack_msg._valid = tmsg._valid;
    ack_msg._content = "";
    const std::string s = ack_msg.ToJson();
    OnTransfer(s);
}

void MRTTransferSession::OnTypeConn(TRANSFERMODULE fmodule, const std::string& str)
{
    CONNMSG c_msg;
    std::string err;
    c_msg.GetMsg(str, err);
    if (err.length() > 0) {
        //connid error
        LE("%s invalid params error\n");
        return;
    }
    if ((c_msg._tag == CONNTAG::co_id) && c_msg._msg.compare("hello") == 0) {
        // when ME connector to other:
        if (c_msg._id.length()>0) {
            m_transferSessId = c_msg._id;
            {
                MRTConnManager::ModuleInfo* pmi = new MRTConnManager::ModuleInfo();
                if (pmi) {
                pmi->flag = 1;
                pmi->othModuleType = fmodule;
                pmi->othModuleId = m_transferSessId;
                pmi->pModule = this;
                //bind session and transfer id
                MRTConnManager::Instance().AddModuleInfo(pmi, m_transferSessId);
                //store which moudle connect to this connector
                MRTConnManager::Instance().AddTypeModuleSession(fmodule, c_msg._moduleid, m_transferSessId);
                LI("store other moduleid:%s, transfersessionid:%s\n", c_msg._moduleid.c_str(), m_transferSessId.c_str());
                } else {
                    LE("new ModuleInfo error\n");
                }
            }
            TRANSFERMSG t_msg;

            t_msg._action = TRANSFERACTION::req;
            t_msg._fmodule = TRANSFERMODULE::mmeeting;
            t_msg._type = TRANSFERTYPE::conn;
            t_msg._trans_seq = GenericTransSeq();
            t_msg._trans_seq_ack = 0;
            t_msg._valid = 1;

            c_msg._tag = CONNTAG::co_msgid;
            c_msg._id = m_transferSessId;
            c_msg._msgid = "ok";
            //send self module id to other
            c_msg._moduleid = MRTConnManager::Instance().MeetingId();

            t_msg._content = c_msg.ToJson();

            std::string s = t_msg.ToJson();
            SendTransferData(s.c_str(), (int)s.length());

        } else {
            LE("Connection id:%s error!!!\n", c_msg._id.c_str());
        }
    }
}

void MRTTransferSession::OnTypeTrans(TRANSFERMODULE fmodule, const std::string& str)
{
    TRANSMSG t_msg;
    std::string err;
    t_msg.GetMsg(str, err);
    if (err.length() > 0) {
        LE("%s TRANSMSG error:%s\n", __FUNCTION__, err.c_str());
        Assert(false);
        return;
    }

    MEETMSG m_mmmsg;
    err = "";
    m_mmmsg.GetMsg(t_msg._content.c_str(), err);
    if (err.length() > 0) {
        LE("%s MEETMSG error:%s\n", __FUNCTION__, err.c_str());
        Assert(false);
        return;
    }
    switch (m_mmmsg._cmd) {
        case MEETCMD::enter:
        case MEETCMD::leave:
        {
            MRTRoomManager::Instance().HandleOptRoom(t_msg, m_mmmsg);
        }
            break;
        case MEETCMD::dcomm:
        {
            //handle msgs
            std::string tos, res;
            MRTRoomManager::Instance().HandleDcommRoom(t_msg, m_mmmsg);
        }
            break;
        default:
        {
            //invalid params
            LE("OnTypeTrans invalid params\n");
        }
            break;
    }
}

void MRTTransferSession::OnTypeQueue(TRANSFERMODULE fmodule, const std::string& str)
{
    LI("%s was called\n", __FUNCTION__);
}

void MRTTransferSession::OnTypeDispatch(TRANSFERMODULE fmodule, const std::string& str)
{
    LI("%s was called\n", __FUNCTION__);
}

void MRTTransferSession::OnTypePush(TRANSFERMODULE fmodule, const std::string& str)
{
    LI("%s was called\n", __FUNCTION__);
}

void MRTTransferSession::OnTypeTLogin(TRANSFERMODULE fmodule, const std::string& str)
{
    TRANSMSG t_msg;
    std::string err;
    t_msg.GetMsg(str, err);
    if (err.length() > 0) {
        LE("%s TRANSMSG error:%s\n", __FUNCTION__, err.c_str());
        Assert(false);
        return;
    }
}

void MRTTransferSession::OnTypeTLogout(TRANSFERMODULE fmodule, const std::string& str)
{
    //the users connection lost
    TRANSMSG t_msg;
    std::string err;
    t_msg.GetMsg(str, err);
    if (err.length() > 0) {
        LE("%s TRANSMSG error:%s\n", __FUNCTION__, err.c_str());
        Assert(false);
        return;
    }
    OnConnectionLostNotify(t_msg._touser, t_msg._content, t_msg._connector);
    return;
}

/**
 *  User connection lost, clear user in the meeting
 */
void MRTTransferSession::OnConnectionLostNotify(const std::string& uid, const std::string& token, const std::string& connector)
{
    MRTRoomManager::Instance().ClearSessionLost(uid, token, connector);
}

/**
 * TransferSession connection lost, clear Transfer Session
 */
void MRTTransferSession::ConnectionDisconnected()
{
    if (m_transferSessId.length()>0) {
        m_connectingStatus = 0;
        MRTConnManager::Instance().TransferSessionLostNotify(m_transferSessId);
    }
}

////////////////////////////////////////////////////////
////////////////////private/////////////////////////////
////////////////////////////////////////////////////////

