#include "tcp_event_server.h"

#include <glog/logging.h>
#include <assert.h>

TcpEventServer::TcpEventServer(int16_t port, int thread_cnt)
    : port_(port), thread_cnt_(thread_cnt)
{
	//初始化各项数据
//	m_MainBase = new LibeventThread;
//	threads_ = new LibeventThread[thread_cnt_];
	main_base_.thread_id = pthread_self();
	main_base_.base = event_base_new();

	//初始化各个子线程的结构体
	for(int i=0; i < thread_cnt_; i++)
	{
		threads_.push_back(new LibeventThread(this));
	}
}

TcpEventServer::~TcpEventServer()
{
	//停止事件循环（如果事件循环没开始，则没效果）
	StopRun(NULL);

	//释放内存
	event_base_free(main_base_.base);
	for(int i=0; i < thread_cnt_; ++i)
        delete threads_[i];
		// threads_[i].FreeEventBase();

//	delete [] threads_;
}

void TcpEventServer::ErrorQuit(const char *str)
{
	//输出错误信息，退出程序
	fprintf(stderr, "%s", str);   
	if( errno != 0 )    
		fprintf(stderr, " : %s", strerror(errno));    
	fprintf(stderr, "\n");        
	exit(1);    
}

bool TcpEventServer::StartRun()
{
	evconnlistener *listener;

	//如果端口号不是EXIT_CODE，就监听该端口号
	if( port_ != EXIT_CODE )
	{
		sockaddr_in sin;
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_port = htons(port_);
		listener = evconnlistener_new_bind(main_base_.base, 
			ListenerEventCallback, this,
			LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE, -1,
			reinterpret_cast<sockaddr*>(&sin), sizeof(sockaddr_in));
		if( NULL == listener )
			ErrorQuit("TCP listen error");
	}

	//开启各个子线程
	for(int i=0; i<thread_cnt_; i++)
	{
        threads_[i]->Start();
	}

	//开启主线程的事件循环
	event_base_dispatch(main_base_.base);

	//事件循环结果，释放监听者的内存
	if( port_ != EXIT_CODE )
	{
		printf("free listen\n");
		evconnlistener_free(listener);
	}
    return true;
}

void TcpEventServer::StopRun(timeval *tv)
{
	//向各个子线程的管理中写入EXIT_CODE，通知它们退出
	for(int i=0; i<thread_cnt_; i++)
	{
        threads_[i]->SignalExit();
	}
	//结果主线程的事件循环
	event_base_loopexit(main_base_.base, tv);
}

void TcpEventServer::ListenerEventCallback(struct evconnlistener *listener, 
									evutil_socket_t fd,
									struct sockaddr *sa, 
									int socklen, 
									void *user_data)
{
	TcpEventServer *server = reinterpret_cast<TcpEventServer*>(user_data);

	//随机选择一个子线程，通过管道向其传递socket描述符
	int num = rand() % server->thread_cnt_;
    server->threads_[num]->ProcessListenerEvent(fd);

}


void TcpEventServer::ReadEventCallback(struct bufferevent *bev, void *data)
{
	Connection *conn = reinterpret_cast<Connection*>(data);
    
	conn->read_buf_ = bufferevent_get_input(bev);
	conn->write_buf_ = bufferevent_get_output(bev);

	//调用用户自定义的读取事件处理函数
    conn->ProcessReadEvent();
} 

void TcpEventServer::WriteEventCallback(struct bufferevent *bev, void *data)
{
	Connection *conn = reinterpret_cast<Connection*>(data);
	conn->read_buf_ = bufferevent_get_input(bev);
	conn->write_buf_ = bufferevent_get_output(bev);

	//调用用户自定义的写入事件处理函数
//	conn->thread_->thread_data_.tcp_event_server->WriteEvent(conn);
    conn->ProcessWriteEvent();
}

void TcpEventServer::CloseEventCallback(struct bufferevent *bev, short events, void *data)
{
	Connection *conn = reinterpret_cast<Connection*>(data);
	//调用用户自定义的断开事件处理函数
//	conn->thread_->thread_data_.tcp_event_server->CloseEvent(conn, events);
    conn->ProcessCloseEvent(events);
	
	bufferevent_free(bev);
}

bool TcpEventServer::AddSignalEvent(int sig, void (*ptr)(int, short, void*))
{
	//新建一个信号事件
	event *ev = evsignal_new(main_base_.base, sig, ptr, this);
	if ( !ev || event_add(ev, NULL) < 0 )
	{
		event_del(ev);
		return false;
	}

	//删除旧的信号事件（同一个信号只能有一个信号事件）
	DeleteSignalEvent(sig);
	signal_events_[sig] = ev;

	return true;
}

bool TcpEventServer::DeleteSignalEvent(int sig)
{
	std::map<int, event*>::iterator iter = signal_events_.find(sig);
	if( iter == signal_events_.end() )
		return false;

	event_del(iter->second);
	signal_events_.erase(iter);
	return true;
}

event *TcpEventServer::AddTimerEvent(void (*ptr)(int, short, void *), 
								  timeval tv, bool once)
{
	short int flag = 0;
	if( !once )
		flag = EV_PERSIST;

	//新建定时器信号事件
	event *ev = new event;
	event_assign(ev, main_base_.base, -1, flag, ptr, this);
	if( event_add(ev, &tv) < 0 )
	{
		event_del(ev);
		return NULL;
	}
	return ev;
}

bool TcpEventServer::DeleteTimerEvent(event *ev)
{
    return  event_del(ev) == 0;
}
