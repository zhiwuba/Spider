#ifndef  __CROTON_SPIDER_EXECUTOR_H__
#define __CROTON_SPIDER_EXECUTOR_H__
#include "spider_url.h"
#include "spider_thread_pool.h"

class Spider_Executor
{
public:
	static Spider_Executor&  instance()
	{
		static Spider_Executor _instance;
		return _instance;
	};
	~Spider_Executor();
	int  initialize();
	int  uninitialize();

	int put_url(UrlPtr url_ptr);
	int put_urls(UrlPtrVec& url_ptrs);

	int execute_loop();
	
private:
	Spider_Executor();
	int              main_thread_aid();
	static  int    main_thread(void* param);
	static  int    worker_work(void* param);
	
	handle_recursivemutex m_queue_mutex;
	std::queue<UrlPtr>      m_task_queue;
	handle_thread              m_thread_handle; 
	handle_semaphore       m_complete;

	Spider_Thread_Pool*  m_thread_pool;

#ifdef WIN32 
	fd_set  m_all_fdset; 
#else
	int  m_epoll_fd;
#endif
	
	volatile bool m_exit;
};


#endif
