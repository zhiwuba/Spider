#include "spider_url_rinse.h"
#include "spider_config.h"
#include "spider_executor.h"
#ifdef WIN32
#include <Ws2tcpip.h>
#endif

typedef struct
{
	Spider_Url_Rinse*  pthis;
	UrlPtr                    url_ptr;
} dns_cb_arg;

static int g_pending_requests=0;

unsigned int domain_hashcode(char* domain)
{
	unsigned int  h=0;
	unsigned int  i=0;
	while (domain[i]!=0)
	{
		h=37*h+domain[i];
		i++;
	}
	return h%kDomainHashSize;
}

Spider_Url_Rinse::Spider_Url_Rinse(void)
{
	m_domain_table=new char[kDomainHashSize/8];
	memset(m_domain_table,0, kDomainHashSize/8);
	m_url_table = new char[kUrlHashSize/8];
	memset(m_url_table,0, kUrlHashSize/8);
	m_history_url_table=new char[kUrlHashSize/8];
	memset(m_history_url_table,0, kUrlHashSize/8);

	m_queue_mutex=recursivemutex_create();

}

Spider_Url_Rinse::~Spider_Url_Rinse(void)
{
	recursivemutex_destory(m_queue_mutex);
	if ( m_domain_table!=NULL )
	{
		delete[] m_domain_table;
		m_domain_table=NULL;
	}
	if ( m_url_table!=NULL )
	{
		delete[] m_url_table;
		m_url_table=NULL;
	}
	if ( m_history_url_table!=NULL )
	{
		delete[] m_history_url_table;
		m_history_url_table=NULL;
	}

}

int  Spider_Url_Rinse::initialize()
{
	if ( Spider_Config::instance().load_history_ )
	{
		read_history();
	}

	m_evbase = event_base_new();	
	if (!m_evbase)
	{
		LLOG(L_ERROR,"event_base_new error.");
		return -1;
	}

#ifdef WIN32
	int init_name_server=0;
#else
	int init_name_server=1;
#endif

	m_evdnsbase = evdns_base_new(m_evbase, init_name_server);
	if (!m_evdnsbase )
	{
		LLOG(L_ERROR,"evdns_base_new error.");
		return -1;
	}

#ifdef WIN32
	int ret=evdns_base_nameserver_ip_add(m_evdnsbase, "8.8.8.8");
	if( ret!=0 )
	{
		LLOG(L_ERROR, "evdns_base_nameserver_add error.");
	}
#endif

	return 0;
}


int Spider_Url_Rinse::uninitialize()
{
	write_history();
	evdns_base_free(m_evdnsbase, 0);
	event_base_free(m_evbase);
	return 0;
}

int  Spider_Url_Rinse::rinse_url(UrlPtr url)
{
	int ret=0;
	if ( url!=NULL )
	{
		UrlPtrVec urlarray;
		urlarray.push_back(url);
		rinse_urls(urlarray);
	}
	else
	{
		LLOG2(L_ERROR,"PushUrl url is null.");
		ret=-1;
	}
	return ret;
}

int Spider_Url_Rinse::rinse_urls(UrlPtrVec& url_array)
{
	Recursive_Lock lock(m_queue_mutex);
	UrlPtrVec  dns_url_array;
	UrlPtrVec  crawl_url_array;

	for (unsigned int i=0 ;i<url_array.size();i++)
	{
		UrlPtr& object=url_array[i];
		if ( !search_and_record_url(object) ) //去除重复的url
		{  //没有检索过此URL
			if ( search_domain(object->domain)) //检查Domain
			{   //在dns数据库中
				std::map<std::string,std::string>::iterator iter=m_dns_database.find(std::string(object->domain));
				if ( iter!=m_dns_database.end() )
				{
					object->ip=strdup((iter->second).c_str());
					crawl_url_array.push_back(object);  //抓取URL
				}
				else
				{
					dns_url_array.push_back(object);
				}
			}
			else
			{   //不在数据库中
				dns_url_array.push_back(object);
			}
		}
	}
	
	LLOG(L_DEBUG, "rinse_urls array size : %d", dns_url_array.size());
	dns_parse(dns_url_array);
	
	Spider_Executor::instance().put_urls(crawl_url_array);

	return 0;
}

void Spider_Url_Rinse::put_url_to_executor(UrlPtr url)
{
	Spider_Executor::instance().put_url(url);
}

bool Spider_Url_Rinse::search_domain(char* domain)
{
	unsigned int code=domain_hashcode(domain);
	unsigned int pos=code/8;
	unsigned int bits=1<<(code%8);
	bool ret=m_domain_table[pos]&bits;
	return ret;
}

void Spider_Url_Rinse::record_domain(char* domain)
{
	unsigned int code=domain_hashcode(domain);
	unsigned int pos=code/8;
	unsigned int bits=1<<(code%8);
	m_domain_table[pos]=m_domain_table[pos]|bits;
}

bool Spider_Url_Rinse::search_and_record_url(UrlPtr url)
{
	unsigned int code=url_hash_code(url);
	unsigned int pos=code/8;
	unsigned int bits=1<<(code%8);
	bool bret=m_url_table[pos]&bits;
	m_url_table[pos]=m_url_table[pos]|bits;
	return bret;	
}


void Spider_Url_Rinse::dns_parse(UrlPtrVec& url_array)
{
	for (unsigned int i = 0; i < url_array.size() ; ++i)
	{
		struct evutil_addrinfo hints;
		struct evdns_getaddrinfo_request *req;
		memset(&hints, 0, sizeof(hints));

		hints.ai_family = AF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = EVUTIL_AI_CANONNAME; 
		hints.ai_protocol = IPPROTO_TCP;

		++g_pending_requests;
		dns_cb_arg *arg = (dns_cb_arg *) calloc(sizeof(dns_cb_arg), 1);
		arg->url_ptr = url_array[i];
		arg->pthis=this;

		req =evdns_getaddrinfo(m_evdnsbase, arg->url_ptr->domain, NULL, &hints, (evdns_getaddrinfo_cb)dns_callback, arg);
		if (req == NULL)
		{
			LLOG(L_WARN,"evdns_getaddrifo return null.");
		}
	}

	if (g_pending_requests)
	{
		int ret=event_base_loop(m_evbase, 0);
		if( ret!=0 )
			LLOG(L_ERROR, "event_base_loop error code %d", ret);
	}
	return;
}

void Spider_Url_Rinse::dns_callback(int errcode, struct evutil_addrinfo *addr, void *ptr)
{
	dns_cb_arg *arg = (dns_cb_arg *)ptr;
	UrlPtr myurl = arg->url_ptr;

	if (errcode)
	{
		LLOG(L_ERROR,"DNS parse error:%s -> %s\n", myurl->domain, evutil_gai_strerror(errcode));
	}
	else
	{
		struct evutil_addrinfo *ai=addr;
		char buf[128];
		const char *s = NULL;
		if (addr->ai_family == AF_INET)
		{
			struct sockaddr_in *sin =(struct sockaddr_in *)ai->ai_addr;
			s = evutil_inet_ntop(AF_INET, &sin->sin_addr, buf, 128);	/*从套接口内部数据类型转换为点分十进制 */
		}
		else if (addr->ai_family == AF_INET6)
		{
			struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)ai->ai_addr;
			s = evutil_inet_ntop(AF_INET6, &sin6->sin6_addr, buf,128);
		}
		if (s)
		{
			myurl->ip=strdup(s);
			arg->pthis->put_url_to_executor(myurl);
			arg->pthis->m_dns_database[std::string(myurl->domain)] = std::string(myurl->ip);
			arg->pthis->record_domain(myurl->domain);
		}
		evutil_freeaddrinfo(addr);
	}

	if (--g_pending_requests == 0)
	{
		event_base_loopexit(arg->pthis->m_evbase, NULL);
	}
}


bool  Spider_Url_Rinse::search_and_record_history(UrlPtr url)
{
	unsigned int code=url_hash_code(url);
	unsigned int pos=code/8;
	unsigned int bits=1<<(code%8);
	bool bret=m_history_url_table[pos]&bits;
	m_history_url_table[pos]=m_history_url_table[pos]|bits;
	return bret;
}


int Spider_Url_Rinse::read_history()
{
	std::string file_path=Spider_Config::instance().history_path_+kHistoryFileName;
	FILE* file=fopen(file_path.c_str(),"rb");
	if ( file!=NULL )
	{
		fseek(file,0, SEEK_END);
		long length=ftell(file);
		fseek(file,0,SEEK_SET);
		if ( length!=kUrlHashSize/8 )
		{
			LLOG(L_ERROR,"spider.history file isn't fit.");
			return -1;
		}
		fread(m_history_url_table, 1, length, file );
		memcpy(m_url_table, m_history_url_table , length);
		fclose(file);
	}
	return 0;
}

int Spider_Url_Rinse::write_history()
{
	std::string file_path=Spider_Config::instance().history_path_+kHistoryFileName;
	FILE* file=fopen(file_path.c_str(), "wb");
	if ( file!=NULL )
	{
		fwrite(m_history_url_table,1, kUrlHashSize/8, file);
		fclose(file);
	}
	return 0;
}
