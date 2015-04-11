#include "spider_database.h"


Spider_Database::Spider_Database()
{
	m_exit=false;
}

Spider_Database::~Spider_Database()
{

}

int Spider_Database::initialize()
{
	if (!mysql_init(&m_mysql) )
	{
		LLOG(L_ERROR,"mysql_init error.");
		return -1;
	}
	else
	{
		int ret=connect();
		if ( ret==0 )
		{
			LLOG(L_TRACE, "connect mysql ok.");
		}
		m_queue_mutex=recursivemutex_create();
		m_thread=thread_create(NULL,0,(THREAD_FUN)thread_proc, this, 0,0);
	}
	return 0;
}

int Spider_Database::uninitialize()
{
	m_exit=true;
	thread_waitforend(m_thread,INFINITE);
	recursivemutex_destory(m_queue_mutex);
	close();
	return 0;
}

int Spider_Database::thread_aid()
{
	while ( !m_exit )
	{
		if ( m_commands.size()>0 )
		{
			std::string command;
			{
				Recursive_Lock lock(m_queue_mutex);
				command=m_commands.front();
				m_commands.pop();
			}
			int ret=query(command.c_str());
			if ( ret!=0 )
			{
				LLOG(L_ERROR,"insert_record error. ");
				return -1;
			}
		}
		else
		{
			Sleep(500);
		}
	}
	return 0;
}

int Spider_Database::thread_proc(void* param)
{
	Spider_Database* pthis=(Spider_Database*)param;
	if ( pthis!=NULL)
	{
		pthis->thread_aid();
	}
	return 0;
}

int Spider_Database::insert_record(const char* website, const char* albums, UrlPtr url_ptr)
{
	Recursive_Lock lock(m_queue_mutex);

	long len=strlen(url_ptr->comment);
	char* es_comment=new char[2*len+1];
	len=mysql_real_escape_string(&m_mysql, es_comment , url_ptr->comment, len);
	if( len>1648 )
	{
		LLOG(L_ERROR, "comment is too long.");
		return -1;
	}

	char command[2048];
	sprintf(command, "INSERT INTO hd_paints (file_name,date_added,header,comment,source_url,parent_url,source_website) VALUES ('%s', NOW(), '%s', '%s', '%s', '%s' ,'%s')",
		url_ptr->filename, 
		"", 
		es_comment,
		url_ptr->url,
		url_ptr->parent,
		website );

	delete[] es_comment;
	m_commands.push(std::string(command));
	return 0;
}

int Spider_Database::connect()
{
	Spider_Config::instance();
	MYSQL* ret=mysql_real_connect(&m_mysql, 
		Spider_Config::instance().mysql_host_.c_str(),
		Spider_Config::instance().mysql_user_.c_str(),
		Spider_Config::instance().mysql_password_.c_str(),
		Spider_Config::instance().mysql_db_.c_str(),
		Spider_Config::instance().mysql_port_,NULL,0);
	if ( ret==NULL )
	{
		LLOG(L_ERROR,"mysql: connect server error!!");
		return -1;
	}
	mysql_set_character_set(&m_mysql , "utf8");
	return 0;
}

int Spider_Database::query(const char* command)
{
	int ret=mysql_real_query(&m_mysql, command, strlen(command));
	if ( ret !=0)
	{
		LLOG(L_ERROR, "query error, code: %s ", mysql_error(&m_mysql));
		return -1;
	}
	return 0;
}

int Spider_Database::close()
{
	mysql_close(&m_mysql);
	return 0;
}

