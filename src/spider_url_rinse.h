#ifndef  __CROTON_SPIDER_URLMANAGER_H__
#define __CROTON_SPIDER_URLMANAGER_H__

#include "event2/dns.h"
#include "event2/util.h"
#include "event2/event.h"

#include "spider_utils.h"
#include "spider_url.h"

//漂洗
class Spider_Url_Rinse
{
public:
	static Spider_Url_Rinse& instance()
	{
		static Spider_Url_Rinse _instance;
		return _instance;
	};

	~Spider_Url_Rinse(void);
	int    initialize();
	int    uninitialize();

	int     rinse_urls(UrlPtrVec& url);
	int     rinse_url(UrlPtr url);

	bool   search_and_record_history(UrlPtr url);

	int      write_history();

private:	
	Spider_Url_Rinse(void);
	static void dns_callback(int errcode, struct evutil_addrinfo *addr, void *ptr);
	
	bool  search_domain(char* domain); //查找域名是否在表中
	void  record_domain(char* domain);  
	void  dns_parse(UrlPtrVec& url_array);

	bool  search_and_record_url(UrlPtr url);     //检查并添加
	
	int read_history();   //history 

	void put_url_to_executor(UrlPtr url);



	struct event_base *m_evbase;
	struct evdns_base *m_evdnsbase;

	std::queue<UrlPtr>  m_queue_ready_crawl;
	handle_recursivemutex m_queue_mutex;

	std::map<std::string, std::string> m_dns_database;
	char*       m_domain_table;   //bloom fliter-->domain
	char*       m_url_table;           //bloom filter-->url
	char*       m_history_url_table; //don't crawl old file.
};

#endif

