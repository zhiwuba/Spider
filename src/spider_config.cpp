#include <ctype.h>
#include "spider_utils.h"
#include "spider_config.h"
#include "spider_common.h"

const char* kSubject[]={"control","storage","cookie", "mysql" ,NULL};
enum SUBJECT
{
	CONTROL,
	STORAGE,
	COOKIE,
	MYSQL
};

Spider_Config::Spider_Config()
{
	load_history_=false;
	load_cookie_=false;
	load_dns_=false;

	storage_path_="";
	dns_path_="";
	cookie_path_="";
	history_path_="";
}



Spider_Config::~Spider_Config()
{

}

int  Spider_Config::load()
{
	module_path_=GetExePath();
	current_date_=get_date();
	std::string config_path=module_path_+kConfigFileName;
	FILE* file=fopen(config_path.c_str(), "r");
	if ( file!=NULL )
	{
		SUBJECT subject;
		std::string site;
		char line[1024];
		while (fgets(line,1024,file))
		{
			if ( line[strlen(line)-1]=='\n' )
			{
				line[strlen(line)-1]='\0';
			}
			char* c=line;
			while (isspace(*c))c++;
			if ( *c=='[' )
			{
				char sub[100];
				int ret=prase_subject(line,sub);
				if ( ret==0 )
				{
					int i=0;
					for (; kSubject[i]!=NULL ;i++)
					{
						if ( strcmp(sub,kSubject[i])==0 )
							break;
					}
					subject=(SUBJECT)i;
				}
			}
			else if ( *c!='\0' )
			{
				std::string key, value;
				int ret=prase_key_value(line, key,value);
				if ( ret==0 )
				{
					if ( subject==CONTROL )
						set_control(key,value);
					else if ( subject==STORAGE )
						set_storage(key,value);
					else if ( subject==COOKIE )
						set_token(site,key,value);
					else if (subject==MYSQL)
						set_mysql(key, value);
				}
			}
		}
	}
	else
	{
		LLOG(L_ERROR,"open config file error.");
		return -1;
	}
	return 0;
}

int Spider_Config::prase_subject(char* line, char* subject)
{
	int ret=-1;
	char* c=line;
	while (isspace(*c))c++;
	if ( *c=='[' )
	{
		char* s=c;
		s++;
		c=line+strlen(line)-1;
		while (isspace(*c))c--;
		if ( *c==']' )
		{
			ret=0;
			strncpy(subject, s, c-s);
			subject[c-s]='\0';
		}
	}
	return ret;
}


int Spider_Config::prase_key_value(char* line, std::string& key, std::string& value)
{
	int ret=-1;
	char* c=line;
	while(isspace(*c))c++;
	char* s=c;
	while (*c!='='&&*c!='\0')c++;
	if ( *c!='\0'&&*c=='=' )
	{
		char* k=c;
		while(isspace(*k))k--;
		key.assign(s, k-s);

		s=line+strlen(line)-1;
		while ( isspace(*s) )s--;
		c++;
		while (isspace(*c))c++;
		
		if ( s>=c )
		{
			value.assign(c, s+1-c);
			ret=0;
		}
	}
	return ret;
}

int  Spider_Config::set_control(std::string& key, std::string& value)
{
	if ( key=="loadhistory" )
	{
		if ( stricmp(value.c_str(),"yes")==0 )
		{
			load_history_=true;
		}
		if ( stricmp(value.c_str(),"no")==0 )
		{
			load_history_=false;
		}
	}
	else if ( key=="loadcookie" )
	{
		if ( stricmp(value.c_str(),"yes")==0 )
		{
			load_cookie_=true;
		}
		if ( stricmp(value.c_str(),"no")==0 )
		{
			load_cookie_=false;
		}
	}
	else if ( key=="loaddns" )
	{
		if ( stricmp(value.c_str(),"yes")==0 )
		{
			load_dns_=true;
		}
		if ( stricmp(value.c_str(),"no")==0 )
		{
			load_dns_=false;
		}
	}
	return 0;
}

int Spider_Config::set_storage(std::string& key, std::string& value)
{
	if ( key=="storage_path" )
	{
		storage_path_=value;
	}
	else if ( key=="dns_path" )
	{
		dns_path_=value;
	}
	else if ( key=="cookie_path" )
	{
		cookie_path_=value;
	}
	else if ( key=="history_path" )
	{
		history_path_=value;
	}
	return 0;
}

int Spider_Config::set_token(std::string& current_site, std::string& key, std::string& value)
{
	if ( current_site.empty()&&key!="site" )
	{
		return -1;
	}
	else
	{
		if ( key=="site" )
		{
			current_site=value;
			Token* token=new Token;
			tokens_[current_site]=token;
		}
		else if ( key=="user" )
		{
			Token* token=tokens_[current_site];
			strcpy(token->account, value.c_str());
		}
		else if ( key=="password")
		{
			Token* token=tokens_[current_site];
			strcpy(token->password, value.c_str());	
		}
	}
	return 0;
}


int  Spider_Config::set_mysql(std::string& key, std::string& value)
{
	if (  key=="host" )
	{
		mysql_host_=value;
	}
	else if ( key=="db" )
	{
		mysql_db_=value;
	}
	else if ( key=="port" )
	{
		mysql_port_=atoi(value.c_str());
	}
	else if ( key=="user" )
	{
		mysql_user_=value;
	}
	else if (key=="password")
	{
		mysql_password_=value;
	}

	return 0;
}
