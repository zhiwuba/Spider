#ifndef  __CROTON_SPIDER_STORAGE_H__
#define __CROTON_SPIDER_STORAGE_H__
#include "spider_database.h"

class Spider_Storage
{
public:
	static Spider_Storage& instance()
	{
		static Spider_Storage _instance;
		return _instance;
	};
	~Spider_Storage();
	
	int initialize();
	int uninitialize();

	int write_file(const char* website, UrlPtr url_ptr );
	//int write_file(const char* website, const char* albums, UrlPtr url_ptr );
	
	int rename_filename_with_md5(UrlPtr url_ptr);

private:
	Spider_Storage();
	Spider_Database* m_database;
};


#endif

