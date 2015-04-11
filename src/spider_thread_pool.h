#ifndef  __BB_THREADPOOL__
#define __BB_THREADPOOL__

#include "spider_url.h"
#include "spider_utils.h"

class Spider_Http_Client;
struct Job
{
	int (*execute_func_)(void*);
	Spider_Http_Client* http_client_;
	int     sock_;
	UrlPtr url_ptr_;
};

class Spider_Thread_Pool
{
public:
	Spider_Thread_Pool(int max_thread_count);
	~Spider_Thread_Pool(void);
	
	void  push_work(void* param);

	int    get_worker_count(); /*获取正在运行的线程数量*/
	int    get_queue_count(); /*获取队列中任务的数量*/

private:
	static unsigned int manage_function(void*);
	
	int      post_semaphore();
	int      wait_semaphore();
	void*  pop_work();
	void   lock_mutex();
	void   unlock_mutex();
	int      run_thread_function(void* work);

	std::list<handle_thread> m_thread_list ;
	std::queue<void*>  m_work_queue;
	
	handle_mutex  m_mutex;
	handle_semaphore  m_semaphore;
	
	volatile bool m_exit_flag;
	volatile int    m_worker_count;
};

#endif
