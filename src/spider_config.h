#ifndef  __CROTON_SPIDER_CONFIG_H__
#define __CROTON_SPIDER_CONFIG_H__
#include "spider_common.h"

struct  Token
{
	char account[100];
	char password[100];
};

class Spider_Config
{
public:
	static Spider_Config&  instance()
	{
		static Spider_Config _instance;
		return _instance;
	};
	~Spider_Config();
	
	int  load();
	
	// Control
	bool  load_history_;
	bool  load_cookie_;
	bool  load_dns_;
	
	//Storage
	std::string  storage_path_;
	std::string  dns_path_;
	std::string  cookie_path_;
	std::string  history_path_;
	
	//Current
	std::string  current_date_; //当前日期
	std::string  module_path_;  //当前目录
	
	//Mysql
	std::string  mysql_host_;
	std::string  mysql_db_;
	int             mysql_port_;
	std::string  mysql_user_;
	std::string  mysql_password_;

	//Token
	std::map<std::string, Token*> tokens_;  //account  password
	
private:
	Spider_Config();

	int  prase_subject(char* line, char* subject);
	int  prase_key_value(char* line, std::string& key, std::string& value);
	int  set_control(std::string& key, std::string& value);
	int  set_storage(std::string& key, std::string& value);
	int  set_token(std::string& current_site, std::string& key, std::string& value);
	int  set_mysql(std::string& key, std::string& value);


};



#endif