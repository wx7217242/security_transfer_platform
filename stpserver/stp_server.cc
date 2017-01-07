#include "stp_server.h"
#include <glog/logging.h>

StpServer& StpServer::GetInstance()
{
    static StpServer s_Instance(9999, 3);
    return s_Instance;
}

void StpServer::QuitStpServerCallback(int sig, short events, void *data)
{ 
    LOG(INFO) << "Catch the SIGINT signal, quit StpServer in one second";
    StpServer *me = reinterpret_cast<StpServer*>(data);
    timeval tv = {1, 0};
    me->StopRun(&tv);
}

void StpServer::HandleConnectionEvent(Connection *conn)
{
    
}

void StpServer::HandleReadEvent(Connection *conn)
{
    
}

void StpServer::HandleWriteEvent(Connection *conn)
{
    
}

void StpServer::HandleCloseEvent(Connection *conn, short events)
{
    
}

void StpServer::HandleProtocol_Ping(Connection *conn, rpc::C2S_Ping *msg)
{
    if (conn == NULL || msg == NULL)
        return;
    
    
}

StpServer::StpServer(int16_t port, int thread_cnt)
    : TcpEventServer(port, thread_cnt)
{
    
}

StpServer::~StpServer()
{
    
}
