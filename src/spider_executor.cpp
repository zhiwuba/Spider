#include "spider_executor.h"
#include "spider_http_client.h"
#include "spider_website.h"
#include "spider_common.h"
#include "spider_url_rinse.h"


Spider_Executor::Spider_Executor()
{
	m_exit=false;
}


Spider_Executor::~Spider_Executor()
{

}

int Spider_Executor::initialize()
{
	m_queue_mutex=recursivemutex_create();
	m_thread_pool=new Spider_Thread_Pool(kThreadPoolSize);
	m_thread_handle=thread_create(NULL,NULL,(THREAD_FUN)main_thread, this,NULL,NULL);
	return 0;
}

int Spider_Executor::uninitialize()
{
	m_exit=true;
	thread_waitforend(m_thread_handle,INFINITE);
	thread_close(m_thread_handle);
	recursivemutex_destory(m_queue_mutex);
	return 0;
}


int Spider_Executor::put_url(UrlPtr url_ptr)
{
	Recursive_Lock lock(m_queue_mutex);
	LLOG(L_TRACE,"Executor: put_url %s. ip: %s", url_ptr->url, url_ptr->ip);
	m_task_queue.push(url_ptr);
	return 0;
}

int Spider_Executor::put_urls(UrlPtrVec& url_ptrs)
{
	Recursive_Lock lock(m_queue_mutex);
	for (int i=0; i<url_ptrs.size(); ++i)
	{
		LLOG(L_TRACE,"Executor: put_urls %s. ip: %s", url_ptrs[i]->url, url_ptrs[i]->ip);
		m_task_queue.push(url_ptrs[i]);
	}
	return 0;
}

int Spider_Executor::execute_loop()
{
	long last_write_time=GetTickCount();
	int count=0;
	while( !m_exit )
	{
		int num=m_thread_pool->get_worker_count();
		bool is_empty=m_task_queue.empty();
		if( num==0 && is_empty )
			count++;
		else
			count=0;
		
		if( count>10 )
		{
			LLOG(L_DEBUG, "spider is idle. ready to exit.");
			break;	
		}
		
		if( GetTickCount()-last_write_time>kRecordHistoryInterval)
		{
			long start=GetTickCount();
			Spider_Url_Rinse::instance().write_history();
			long cost=GetTickCount()-start;
			LLOG(L_TRACE, "execute_loop write_history cost %ld. ", cost);
			last_write_time=GetTickCount();
		}
		
		Sleep(1000*10);
	}
	return 0;
}

int Spider_Executor::main_thread_aid()
{
	int sock_count=0;
	int close_count=0;
	Spider_Http_Client http_client;
	std::map<int, UrlPtr> socket_url_map;

#ifdef WIN32

	FD_ZERO(&m_all_fdset);
	
	while  (!m_exit)
	{
		//FD_SET()  FD_CLR()  FD_ISSET()  FD_ZERO()
		int maxfds=0;
		
		if ( m_all_fdset.fd_count< kMinSet && m_thread_pool->get_queue_count()<4)
		{
			int count=0;
			Recursive_Lock lock(m_queue_mutex);
			while(count<kProcessCountPer&&m_task_queue.size()>0)
			{
				UrlPtr url_ptr=m_task_queue.front();
				m_task_queue.pop();
				count++;
				int sock=http_client.send_request(url_ptr);
				if ( sock>0 )
				{
					sock_count++;
					FD_SET(sock,&m_all_fdset);
					socket_url_map[sock]=url_ptr;
				}//if
			}//while

			LLOG(L_TRACE, "m_all_fdset socket count is %d, task_queue size is %d", m_all_fdset.fd_count, m_task_queue.size());
		}//if
	
		if ( m_all_fdset.fd_count>0  )
		{	//数量大于0
			fd_set fd_read=m_all_fdset;
			int ret=select(maxfds,&fd_read,NULL,NULL,NULL );
			switch(ret)
			{
			case SOCKET_ERROR:
				LLOG(L_ERROR,"select error,code %d.", lasterror);
				break;
			case 0:  //超时
				LLOG(L_ERROR,"select timeout,code %d.", lasterror);
				break;
			default: //有多个
				{
					int  fd_count=(int)fd_read.fd_count;
					for(int i=0; i<fd_count; i++)
					{
						if(FD_ISSET(fd_read.fd_array[i], &m_all_fdset))
						{
							Job* job=new Job;
							job->execute_func_=worker_work;
							job->url_ptr_=socket_url_map[fd_read.fd_array[i]];
							job->http_client_=&http_client;
							job->sock_=fd_read.fd_array[i];
							m_thread_pool->push_work(job);
							close_count++;
							FD_CLR(fd_read.fd_array[i] , &m_all_fdset);
						}
						else
						{
							LLOG(L_ERROR, "fd_read is not in m_all_fdset");
						}
					}//for
				}
				break;
			}//switch
		}//if
		
		LLOG(L_TRACE, "socket_count is %d, close_count is %d.", sock_count, close_count);
		Sleep(500);
	}//while

#endif

#ifdef LINUX
	//linux  epoll
	
	m_epoll_fd=epoll_create(65536);
	epoll_event events[1000];
	while(!m_exit)
	{
		{
			int count=0;
			Recursive_Lock lock(m_queue_mutex);
			while(count<kProcessCountPer&&m_task_queue.size()>0)
			{
				UrlPtr url_ptr=m_task_queue.front();
				m_task_queue.pop();
				int sock=http_client.send_request(url_ptr);
				if ( sock>0 )
				{
					sock_count++;
					struct epoll_event event;
					event.data.fd=sock;
					event.events=EPOLLIN|EPOLLERR;
					epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, sock, &event);
					socket_url_map[sock]=url_ptr;
				}//if
			}//while
		}
		
		int timeout=100; //ms
		int ndfs=epoll_wait(m_epoll_fd, events, 1000, timeout);
		for( int i=0; i<ndfs; i++ )
		{
			int sock=events[i].data.fd;
			if( events[i].events & EPOLLIN )
			{  //can ready
				Job* job=new Job;
				job->execute_func_=worker_work;
				job->url_ptr_=socket_url_map[sock];
				job->http_client_=&http_client;
				job->sock_=sock;
				m_thread_pool->push_work(job);
				//delete epoll
				epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);
			}
			else
			{   //error delete epoll
				LLOG(L_ERROR, "delete error sock in epoll.");
				epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, NULL);	
			}
		}

		LLOG(L_TRACE, "socket count is %d, close_count is %d");
	}

#endif 
	
	return 0;
}


int Spider_Executor::main_thread(void* param)
{
	Spider_Executor* pthis=(Spider_Executor*)param;
	pthis->main_thread_aid();	
	return 0;
}


int Spider_Executor::worker_work(void* param)
{
	Job* job=(Job*)param;
	if ( job!=NULL )
	{
		UrlPtr& url_ptr=job->url_ptr_; 
		int status_code=job->http_client_->recv_response(job->sock_,&url_ptr->response, url_ptr->length);
		if ( status_code==200 )
		{
			if ( url_ptr->belong!=NULL && url_ptr->length>0 )
			{
				((Spider_WebSite*)url_ptr->belong)->process(url_ptr);
			}
		}
		else if (status_code==302 && url_ptr->response!=NULL && strncmp(url_ptr->response,"http://",7)==0 )
		{
			UrlPtr new_url_ptr=create_url(url_ptr->response, url_ptr->type);
			new_url_ptr->belong=url_ptr->belong;
			Spider_Executor::instance().put_url(new_url_ptr);
		}
		else if( status_code==-1 || url_ptr->response==NULL )
		{
			//重新下载
			UrlPtr new_url_ptr=create_url(url_ptr->url, url_ptr->type);
			new_url_ptr->belong=url_ptr->belong;
			Spider_Executor::instance().put_url(new_url_ptr);
		}
	}

	return 0;
}
