#include "spider_common.h"
#include "spider_config.h"
#include "spider_seed.h"
#include "spider_website.h"
#include "spider_executor.h"
#include "spider_storage.h"
#include "spider_url_rinse.h"
#include "spider_cookie.h"


int main()
{
	int ret=init_network();
	if( ret!=0 )
	{
		LLOG(L_ERROR, "init_network error. ");
		return -1;
	}
	
	Spider_Config::instance().load();
	Spider_Seed spider_seed;
	if( 0!=spider_seed.load() )
	{
		return -1;
	}
	
	Spider_Cookie::instance().login();
	Spider_Storage::instance().initialize();
	Spider_Executor::instance().initialize();
	Spider_Url_Rinse::instance().initialize();

	while(true)
	{
		Seed* seed=NULL;
		std::string site_name;
		spider_seed.get_seed(site_name,&seed);
		if ( seed!=NULL )
		{
            LLOG(L_DEBUG, "load seed %s", site_name.c_str());
			Spider_WebSite* website=Spider_WebSite_Factory::create_website(site_name);
			website->initialize(site_name.c_str(), seed);
			website->begin_process();  //处理seed节点
		}
		else
		{
			LLOG(L_DEBUG, "load all seed.");
			break;
		}
		Sleep(1000); //1s
	}
	
	Spider_Executor::instance().execute_loop(); //循环等待完成
	
	Spider_Url_Rinse::instance().uninitialize();
	Spider_Executor::instance().uninitialize();
	Spider_Storage::instance().uninitialize();
	uninit_network();
	return 0;
};

