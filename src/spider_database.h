#ifndef  __CROTON_SPIDER_DATABASE_H__
#define __CROTON_SPIDER_DATABASE_H__
#include "spider_utils.h"
#include "spider_config.h"
#include "spider_url.h"
#include <mysql/mysql.h>

#ifdef WIN32
#pragma  comment(lib,"libmysql.lib")
#endif

class Spider_Database
{
public:
	Spider_Database();
	~Spider_Database();
	
	int initialize();  
	int uninitialize();
	
	int insert_record(const char* website, const char* albums, UrlPtr url_ptr);
	
	
private:
	int    thread_aid();
	static int thread_proc(void* param);
	int connect();
	int query(const char* command);
	int close();

	handle_thread               m_thread;
	handle_recursivemutex  m_queue_mutex;
	std::queue<std::string> m_commands;
	MYSQL  m_mysql;

	volatile  int m_exit;
};




#endif

