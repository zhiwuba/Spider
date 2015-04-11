#include "spider_thread_pool.h"


unsigned int Spider_Thread_Pool::manage_function(void* argv)
{
	Spider_Thread_Pool* pThis=(Spider_Thread_Pool*)argv;
	if ( pThis!=NULL )
	{
		LLOG(L_TRACE,"EnterInto ManageFunction. ");
		while(pThis->m_exit_flag!=true)
		{
			pThis->wait_semaphore(); //线程阻塞 等待信号
			pThis->lock_mutex();

			void* work=pThis->pop_work();
			pThis->unlock_mutex();	
			pThis->m_worker_count++;
			pThis->run_thread_function(work);
			pThis->m_worker_count--;
		}
		LLOG(L_TRACE,"ExitFrom ManageFunction.");
	}
	return 0;
}


Spider_Thread_Pool::Spider_Thread_Pool( int max_thread_count )
{
	m_worker_count=0;
	m_semaphore=semaphore_create(0,max_thread_count);
	m_mutex=mutex_create();
	m_exit_flag=false;
	for ( int i=0; i<max_thread_count; i++  )
	{
		handle_thread handle=thread_create(NULL,NULL,(THREAD_FUN)manage_function,this,NULL,NULL);
		if ( handle!=NULL )
		{
			m_thread_list.push_back(handle);
		}
	}
}

Spider_Thread_Pool::~Spider_Thread_Pool(void)
{
	m_exit_flag=true;
	for (unsigned int i=0; i<m_thread_list.size(); i++ )
	{
		semaphore_release(m_semaphore);
	}

	LLOG(L_DEBUG,"release threadpool .");
	std::list<handle_thread>::iterator iter=m_thread_list.begin();
	for ( ; iter!=m_thread_list.end() ; iter++ )
	{
	    handle_thread handle=*iter;
		thread_waitforend(handle, INFINITE );
		thread_close(handle);
	}

	LLOG(L_DEBUG,"destroy semphore and mutex .");
	semaphore_destory(m_semaphore);
	m_semaphore=NULL;

	mutex_destroy(m_mutex);
	m_mutex=NULL;

    LLOG(L_DEBUG,"ThreadPool Exit....");
}

int Spider_Thread_Pool::run_thread_function(void* work)
{
	int ret=-1;
	Job* job=(Job*)work;
	if (job!=NULL&&job->execute_func_!=NULL )
	{
		ret=job->execute_func_(job);
	}
	return ret;
}

void Spider_Thread_Pool::push_work(void* sock)
{
	lock_mutex();
	m_work_queue.push(sock);
	post_semaphore();
	unlock_mutex();
}

void*  Spider_Thread_Pool::pop_work()
{
	if ( m_work_queue.size()>0 )
	{
		void* val= m_work_queue.front();
		m_work_queue.pop();
		return val;
	}
	else
	{
		return NULL;
	}
}

int Spider_Thread_Pool::wait_semaphore()
{
	int ret=semaphore_wait(m_semaphore);
	return ret;
}

int Spider_Thread_Pool::post_semaphore()
{
	int ret=semaphore_release(m_semaphore);
	return ret;
}

void Spider_Thread_Pool::lock_mutex()
{
	mutex_lock(m_mutex);
}

void Spider_Thread_Pool::unlock_mutex()
{
	mutex_unlock(m_mutex);
}

int Spider_Thread_Pool::get_worker_count()
{
	return m_worker_count;
}

int Spider_Thread_Pool::get_queue_count()
{
	return m_work_queue.size();
}
