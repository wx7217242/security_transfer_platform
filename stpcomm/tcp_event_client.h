#ifndef TCP_EVENT_CLIENT_H_
#define TCP_EVENT_CLIENT_H_

#include "event_notifier.h"

class TcpEventClient  : public EventNotifier
{
public:
    //新建连接成功后，会调用该函数
	virtual void HandleConnectionEvent(Connection *conn) { }

	//读取完数据后，会调用该函数
	virtual void HandleReadEvent(Connection *conn) { }

	//发送完成功后，会调用该函数（因为串包的问题，所以并不是每次发送完数据都会被调用）
	virtual void HandleWriteEvent(Connection *conn) { }

	//断开连接（客户自动断开或异常断开）后，会调用该函数
	virtual void HandleCloseEvent(Connection *conn, short events) { }

	//发生致命错误（如果创建子线程失败等）后，会调用该函数
	//该函数的默认操作是输出错误提示，终止程序
	virtual void HandleErrorQuit(const char *str) { }
    
private:
    Connection* conn_;
};

#endif