#include "spider_cookie.h"
#include "spider_http_client.h"
#include "spider_utils.h"
#include "spider_config.h"
#include "mpirxx.h"


Cookie::Cookie()
{
	m_mutex=mutex_create();
}

Cookie::~Cookie()
{
	if ( m_mutex!=NULL )
	{
		mutex_destroy(m_mutex);
	}
}

std::string& Cookie::operator()(std::string key)
{
	std::map<std::string,std::string>::iterator iter;
	mutex_lock(m_mutex);
	iter=m_cookie_map.find(key);
	if ( iter==m_cookie_map.end() )
	{
		m_cookie_map.insert(make_pair(key,""));
		iter=m_cookie_map.find("key");
	}
	mutex_unlock(m_mutex);

	return iter->second;
}

void  Cookie::set_cookie(std::string cookie)
{
	if ( !cookie.empty() )
	{
		StrVec result;
		split_string(cookie,result,";");
		mutex_lock(m_mutex);
		for (unsigned int i=0;i<result.size(); i++)
		{
			std::string original=result[i];
			int pos=original.find("=");
			if ( pos!=std::string::npos )
			{
				std::string key=original.substr(0,pos);
				std::string value=original.substr(pos+1);
				m_cookie_map[key]=value;
			}
		}
		mutex_unlock(m_mutex);
	}
}

std::string Cookie::to_string()
{
	std::string cookie_value;
	bool first=true;
	mutex_lock(m_mutex);
	std::map<std::string,std::string>::iterator iter=m_cookie_map.begin();
	for ( ; iter!=m_cookie_map.end() ;iter++)
	{
		std::string temp=iter->first+"="+iter->second;
		if ( first==false )
		{
			cookie_value+="; ";
			cookie_value+=temp;
		}
		else
		{
			cookie_value+=temp;
			first=false;
		}
	}
	mutex_unlock(m_mutex);
	return cookie_value;
}

Cookie* Cookie::from_string(std::string content)
{
	Cookie* cookie=NULL;
	if ( content.length()>0 )
	{
		cookie=new Cookie();
		cookie->set_cookie(content);
	}
	return cookie;
}

Spider_Cookie::Spider_Cookie()
{
}


Spider_Cookie::~Spider_Cookie()
{
}

int Spider_Cookie::login()
{
	int nret=0;
	if ( load_cookie()!=0 )
	{
		int ret1= 0;
			/*Spider_Cookie::instance().login_renren(
			Spider_Config::instance().tokens_["renren"]->account,
			Spider_Config::instance().tokens_["renren"]->password); */
		
		int ret2=Spider_Cookie::instance().login_weibo(
			Spider_Config::instance().tokens_["weibo"]->account,
			Spider_Config::instance().tokens_["weibo"]->password);

		if ( ret1==0&&ret2==0 )
		{
			save_cookie();
		}
		else
		{
			nret=-1;
		}
	}
	return nret;
}


Cookie* Spider_Cookie::get_cookie(std::string key)
{
	Cookie* cookie=NULL;
	std::map<std::string, Cookie*>::iterator iter=m_cookie_database.find(key);
	if ( iter!=m_cookie_database.end() )
	{
		cookie=iter->second;
	}
	return cookie;
}

//renren\ncookie\n 
int Spider_Cookie::save_cookie()
{
	int nret=0;
	std::string filename=Spider_Config::instance().cookie_path_+KCookieFileName;
	FILE* file=fopen(filename.c_str(),"wb");
	if ( file!=NULL )
	{
		std::map<std::string, Cookie*>::iterator  iter=m_cookie_database.begin();
		for ( ;iter!=m_cookie_database.end(); iter++ )
		{
			std::string key=iter->first+"\n";
			std::string value=iter->second->to_string()+"\n";
			fwrite(key.c_str(),1, key.size(), file);
			fwrite(value.c_str(),1, value.size(), file);
		}
		fclose(file);
	}
	else
	{
		LLOG2(L_ERROR , "SaveCookie fopen error.");
		nret=-1;
	}
	return nret;
}


int Spider_Cookie::load_cookie()
{
	int nret=0;
	std::string filename=Spider_Config::instance().cookie_path_+KCookieFileName;
	FILE* file=fopen(filename.c_str() , "rb");
	if ( file!=NULL )
	{
		fseek(file,0, SEEK_END);
		long length=ftell(file);
		char* content=new char[length+1];
		fseek(file,0,SEEK_SET);
		int ret=fread(content,1, length, file );
		content[length]='\0';
		StrVec key_value;
		split_string(std::string(content), key_value ,"\n");
		if ( key_value.size()%2!=0 )
		{
			LLOG2(L_ERROR,"LoadCookie file's format error.");
			nret=-1;
		}
		else
		{
			for (int i=0; i<key_value.size(); i=i+2 )
			{
				std::string key=key_value[i];
				std::string value=key_value[i+1];
				Cookie* cookie=Cookie::from_string(value);
				m_cookie_database[key]=cookie;
			}
		}

		fclose(file);
	}
	else
	{
		LLOG(L_WARN,"Can't find cookie.record,maybe deleted.");
		nret=-1;
	}
	return nret;
}

//----------------------------------------------------------
int Spider_Cookie::login_renren(const char* account, const char* password)
{
	LLOG(L_TRACE,"Start to LoginRenRen. ");
	Cookie* renren_cookie=new Cookie();

	Json::Value encrypt_key;
	int ret=renren_getencryptkey( encrypt_key,renren_cookie);
	if ( ret!=0 )
	{
		delete renren_cookie;
		LLOG(L_ERROR,"getEncryptKey Error.");
		return -1;
	}
	LLOG(L_TRACE,"RenRen_GetEncryptKey OK.");

	std::string code;
	if ( renren_showcaptcha(std::string(account), renren_cookie) )
	{
		renren_getcaptcha(code, renren_cookie);
	}
	LLOG(L_TRACE,"RenRen_ShowCaptcha OK.");
	
	std::string homeurl;
	ret=renren_ajaxlogin(account,password,code.c_str() , encrypt_key, renren_cookie, homeurl);
	if ( ret!=0 )
	{
		delete renren_cookie;
		LLOG(L_ERROR,"Ajax_Login Error.");
		return -1;
	}
	
	LLOG(L_TRACE, "Sucess to login renren.");
	//std::string token=RenRen_GetToken(homeurl, renren_cookie);
	
	m_cookie_database["renren"]=renren_cookie;
	return 0;
}


int Spider_Cookie::renren_getencryptkey(Json::Value& encrypt_key, Cookie* cookie)
{
	Http_Request_Package   request_package;
	Http_Response_Package response_package;
	request_package.set_request_line("GET /ajax/getEncryptKey HTTP/1.1");
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host", "login.renren.com");
	int ret=surface_page("login.renren.com", request_package, response_package);
	if (  ret!=200&&ret!=302 )
	{
		LLOG(L_ERROR, "getEncryptKey::surface_page  error.");
		return -1;
	}

	cookie->set_cookie(response_package.get_field("Cookie"));

	char* temp;
	ret=response_package.get_body(temp);
	std::string body(temp, ret);
	Json::Reader  reader;
	if ( false==reader.parse(body, encrypt_key))
	{
		LLOG(L_ERROR,"getEncryptKey::JsonPrase Error.");
		return -1;
	}
	return 0;
}

int Spider_Cookie::renren_showcaptcha( std::string email ,Cookie* cookie)
{  //验证码
	std::string data="email="+email;
	Http_Request_Package   request_package;
	Http_Response_Package response_package;
	request_package.set_request_line("POST /ajax/ShowCaptcha HTTP/1.1");
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host", "www.renren.com");
	request_package.set_field("Cookie", cookie->to_string());
	request_package.set_field("Content-Length", data.length() );
	request_package.set_body(data);
	int ret=surface_page("www.renren.com", request_package, response_package);
	if (  ret!=200&&ret!=302 )
	{
		LLOG(L_ERROR, "ShowCaptcha::surface_page  error.");
		return -1;
	}
	
	cookie->set_cookie(response_package.get_field("Cookie"));

	char* temp=NULL;
	ret=response_package.get_body(temp);
	int isCode=atoi(temp);
	return isCode;
}


int Spider_Cookie::renren_getcaptcha(std::string& code, Cookie* cookie)
{
	Http_Request_Package request_package;
	Http_Response_Package response_package;
	request_package.set_request_line("GET /getcode.do?t=web_login&rnd=Math.random() HTTP/1.1");
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host", "icode.renren.com");
	request_package.set_field("Connection","Keep-Alive");
	request_package.set_field("Cookie", cookie->to_string());
	int ret=surface_page("icode.renren.com", request_package, response_package);
	if (  ret!=200&&ret!=302 )
	{
		LLOG(L_ERROR, "GetCaptcha surface_page error.");
		return -1;
	}
	FILE* file=fopen("renren_icode.jpg","w");
	if ( file!=NULL )
	{
		char* temp=NULL;
		ret=response_package.get_body(temp);
		fwrite(temp,1,ret, file);
		fclose(file);
		LLOG(L_TRACE,"Please Input Renren captcha.");
		std::cin>>code;
	}
	return 0;
}

std::string Spider_Cookie::renren_gettoken( std::string url, Cookie* cookie)
{
	Http_Request_Package request_package;
	Http_Response_Package response_package;
	std::string host, uri, line;
	int port, ret;

	do 
	{   //处理重定向
		request_package.clear();
		response_package.clear();
		prase_url(url,host,uri,port);
		line="GET "+uri+" HTTP/1.1";
		request_package.set_request_line(line);
		request_package.set_field("Accept","*/*");
		request_package.set_field("User-Agent","Mozilla/5.0");
		request_package.set_field("Host", host);
		request_package.set_field("Cookie",cookie->to_string());
		ret=surface_page(host, request_package, response_package);
		if (  ret!=200&&ret!=302 )
		{
			LLOG(L_ERROR, "getEncryptKey::surface_page  error.");
			return "";
		}
		cookie->set_cookie(response_package.get_field("Set-Cookie"));
		if ( ret==302 )
		{
			char*  temp;
			int len=response_package.get_body(temp);
			std::string body(temp, len);
			int begin_pos=body.find("<a href=\"");
			int end_pos=body.find("\">", begin_pos);
			url=body.substr(begin_pos+9, end_pos-begin_pos-9);
		}
	} while (ret==302);
   
	std::string token;
	char* body;
	int len=response_package.get_body(body);
	std::string strbody(body,len);
	int pos=strbody.find("get_check_x:'");
	if ( pos!=std::string::npos )
	{
		int pos_end=strbody.find('\'', pos+13 );
		token=strbody.substr(pos+13, pos_end-pos-13);
	}

	return token;
}

std::string Spider_Cookie::renren_encryptpwd(const char* e, const char* n, const char* pwd)
{
	mpz_class e_object(e , 16);
	mpz_class n_object(n,  16);
	std::string str_e=e_object.get_str(10);
	std::string str_n=n_object.get_str(10);
	n_object.set_str(str_n,10);

	StrVec trunks;
	std::string strpwd(pwd);
	for (unsigned int i=0; i<strpwd.size(); i++  )
	{
		std::string temp=strpwd.substr(0, 30);
		i+=temp.size();
		trunks.push_back(temp);
	}
	
	std::string result;
	for (unsigned int i=0;i<trunks.size(); i++  )
	{
		std::string temp=trunks[i];
		std::vector<int> trunk;
		for (unsigned int i=0; i<temp.size(); i++  )
		{
			trunk.push_back( (int)temp.at(i) );
		}
		if ( trunk.size()%2!=0 )
		{
			trunk.push_back(0);
		}
		std::vector<int> nums;
		for (unsigned int i=0; i<trunk.size(); i+=2 )
		{
			int num=trunk[i]+(trunk[i+1]<<8);
			nums.push_back(num);
		}
		
		mpz_class  sum=0;
		for (unsigned int i=0 ;i<nums.size(); i++ )
		{
			mpz_class num(nums[i]);
			sum+=num<<(i*16);
		}
		
		mpz_class  pow_result=1;
		int int_e=atoi(str_e.c_str());
		for ( int i=0;i<int_e;i++ )
		{
			pow_result*=sum;
			pow_result%=n_object;
		}
		pow_result=pow_result%n_object;
		std::string ret=pow_result.get_str(16);
		result+=ret;
	}
	
	return result;
}

int Spider_Cookie::renren_ajaxlogin(const char* email, const char* password ,const char* code, Json::Value& encrypt_key, Cookie* cookie, std::string& home_url)
{
	std::string encrypt_pwd=renren_encryptpwd(encrypt_key["e"].asCString(), encrypt_key["n"].asCString(),password );
	char post_data[4096];
	sprintf(post_data,
		"email=%s&icode=%s&origURL=www.renren.com&domain=renren.com&key_id=1&captcha_type=web_login&password=%s&&rkey=%s",
		 email , code, encrypt_pwd.c_str(), encrypt_key["rkey"].asCString()
		 );
	
	Http_Request_Package request_package;
	Http_Response_Package response_package;
	char line[200];
	sprintf(line,"POST /ajaxLogin/login?1=1&uniqueTimestamp=2013%ld HTTP/1.1",GetTickCount());
	request_package.set_request_line(std::string(line));
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host", "www.renren.com");
	request_package.set_field("Content-Length", strlen(post_data));
	request_package.set_field("Content-Type", "application/x-www-form-urlencoded");
	request_package.set_field("Cookie", cookie->to_string());
	request_package.set_body(std::string(post_data));

	int ret=surface_page("www.renren.com", request_package, response_package);
	if ( ret!=200&&ret!=302 )
	{
		LLOG(L_ERROR, "surface_page ajaxLogin error.");
		return -1;
	}
	cookie->set_cookie(response_package.get_field("Set-Cookie"));
	
	char* rsp_body=NULL;
	response_package.get_body(rsp_body);
	Json::Value result;
	Json::Reader reader;
	if ( reader.parse(std::string(rsp_body),result) )
	{
		if ( result["code"].asString().compare("true")==0)
		{
			LLOG(L_TRACE,"Login renren sucess.");
			home_url=result["homeUrl"].asString();
			return 0;
		}
		else
		{
			LLOG(L_ERROR,"Login error, %s", result["failDescription"].asCString());
		}
	}
	return -1;
}

/////////////////////////新浪微博///////////////////////////////////

int Spider_Cookie::login_weibo(const char* account, const char* password)
{
	LLOG(L_DEBUG, "login_weibo: account: %s password:%s", account, password);
	Cookie* cookie=new Cookie();
	std::string ec_account=url_encode(std::string(account));
	std::string str_temp=base64_encrypt(ec_account.c_str(), ec_account.size());
    std::string en_account=url_encode(str_temp);

	std::string servertime;
	std::string nonce;
	std::string rsakv;
	std::string pubkey;
	std::string pcid;
	bool         showpin;
	int ret=sina_preloginstatus(en_account,servertime, nonce, rsakv, pubkey, pcid, showpin ,cookie);
	if ( ret!=0 )
	{
		LLOG(L_ERROR, "LoginWeibo-->Sina_PreloginStatus Failed!!!!!");
		return -1;
	}
	
	std::string str_build=servertime+'\t' +nonce+'\n'+password;
	const char* hex_n=pubkey.c_str();
	const char* hex_e="10001";
	mpz_class  big_n, big_e;
	big_n.set_str(hex_n, 16);
	big_e.set_str(hex_e, 16);
	std::string dec_n=big_n.get_str(10);
	std::string dec_e=big_e.get_str(10); 
	std::string en_password=RSA_encrypt(str_build.c_str() , dec_n.c_str(), dec_e.c_str() );
	std::string  weibo_url;
	
	std::string verification;
	if( showpin )
	{  //获取验证码
		sina_codes(pcid, cookie, verification);
	}
 
	ret=sina_dologin(en_account ,en_password,servertime, nonce, rsakv, weibo_url,verification,cookie);
	if ( ret!=0 )
	{
		LLOG(L_ERROR,"sina_dologin login error!!");
		delete cookie;
		return -1;
	}
	else 
	{
		LLOG(L_DEBUG, "sina_dologin login success.");
		m_cookie_database["weibo"]=cookie;
		return 0;
	}
}

int Spider_Cookie::sina_preloginstatus(std::string en_account, std::string& servertime, 
									   std::string& nonce, std::string& rsakv,std::string& pubkey, std::string& pcid, bool& showpin, Cookie* cookie)
{
	LLOG(L_TRACE, "Weibo_PreloadStatus.");
	std::string request_line="GET /sso/prelogin.php?entry=weibo&callback=sinaSSOController.preloginCallBack&su=";
	request_line+=en_account;
	request_line+="&rsakt=mod&checkpin=1&client=ssologin.js(v1.4.18) HTTP/1.1";
	
	Http_Request_Package    request_package;
	Http_Response_Package  response_package;
	request_package.set_request_line(request_line);
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host","login.sina.com.cn");
	int ret=surface_page("login.sina.com.cn",request_package, response_package);
	if ( ret!=200&&ret!=302 )
	{
		LLOG2(L_ERROR,"Weibo_PreloadStatus surface_page error. ");
		return -1;
	}

	cookie->set_cookie(response_package.get_field("Set-Cookie"));
	
	char* rsp_body=NULL;
	int length=response_package.get_body(rsp_body);
	std::string body;
	body.assign(rsp_body, length);
	int pos1=body.find("(");
    int pos2=body.find_last_of(")");
	if ( pos1>=pos2||body.empty() )
	{
		LLOG2(L_ERROR,"Weibo_PreloadStatus recvbody error." );
		return -1;
	}

	body=body.substr(pos1+1, pos2-pos1-1);
	Json::Reader reader;
	Json::Value   data;
	if ( reader.parse(body,data) )
	{
		char temp[30];
		sprintf(temp,"%d", data["servertime"].asInt());
		servertime.assign(temp);
		nonce=data["nonce"].asString();
		rsakv=data["rsakv"].asString();
		pubkey=data["pubkey"].asString();
		pcid=data["pcid"].asString();
		showpin=data["showpin"].asBool();
	}
	else
	{
		LLOG2(L_ERROR,"Weibo_PreloadStatus response content is invalid.");
		return -1;
	}
	return 0;
}

int Spider_Cookie::sina_dologin(std::string& en_acount ,std::string& en_password,std::string& servertime,
								std::string& nonce, std::string&rsakv, std::string& weibo_url, std::string& verification, Cookie* cookie)
{
	char post_data[4096];
	std::string from_url=url_encode("http://weibo.com/ajaxlogin.php?framelogin=1&callback=parent.sinaSSOController.feedBackUrlCallBack");
	sprintf(post_data,
		"entry=weibo&gateway=1&from=&savestate=7&useticket=1&ssosimplelogin=1&pagerefer=&%svsnf=1&su=%s&service=miniblog&servertime=%s&nonce=%s&pwencode=rsa2&rsakv=%s&sp=%s&encoding=UTF-8&url=%s&returntype=META",
		verification.c_str() , en_acount.c_str(), servertime.c_str(), nonce.c_str(), rsakv.c_str(), en_password.c_str(), from_url.c_str());
	
	Http_Request_Package    request_package;
	Http_Response_Package  response_package; 
	request_package.set_request_line("POST /sso/login.php?client=ssologin.js(v1.4.18) HTTP/1.1");
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host",  "login.sina.com.cn");
	request_package.set_field("Content-Length",strlen(post_data));
	request_package.set_field("Content-Type","application/x-www-form-urlencoded");
	request_package.set_field("Cookie", cookie->to_string());
	request_package.set_body(std::string(post_data));
	int ret=surface_page("login.sina.com.cn", request_package, response_package);
	if ( ret!=200&&ret!=302 )
	{
		LLOG2(L_ERROR , "sina_dologin surface_page error. ");
		return -1;
	}
	else
	{
		cookie->set_cookie(response_package.get_field("Set-Cookie"));
	}

	char* body=NULL;
	ret=response_package.get_body(body);
	std::string recv_data;
	recv_data.assign(body, ret);
	if( recv_data.find("retcode\":0")!=std::string::npos )
	{	//登陆成功
		return 0;
	}
	else
	{  //错误
		return -1;
	}
}

int Spider_Cookie::sina_codes(std::string pcid, Cookie* cookie, std::string& verification)
{   
	Http_Request_Package    request_package;
	Http_Response_Package  response_package; 
	
	std::string request_line="GET /cgi/pin.php?r=28407888&s=0&p="+pcid+" HTTP/1.1";
	request_package.set_request_line(request_line);
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host",  "login.sina.com.cn");
	request_package.set_field("Cookie",cookie->to_string() );
	int ret=surface_page("login.sina.com.cn", request_package, response_package);
	if ( ret!=200&&ret!=302 )
	{
		LLOG2(L_ERROR , "sina_codes surface_page error. ");
		return -1;
	}
	else
	{
		cookie->set_cookie(response_package.get_field("Set-Cookie"));
	}
	
	char* body=NULL;
	int length=response_package.get_body(body);
	std::string filename=GetExePath()+PATH_SEPARATOR+"weibo_code.png";
	FILE* file=fopen(filename.c_str(), "wb");
	if ( file!=NULL )
	{
		fwrite(body,1, length, file);
		fclose(file);
	}
	
	LLOG(L_WARN,"========Please input the weibo code.============");
	std::string code;
	std::cin>>code;
	verification="pcid="+pcid+"&door="+code+"&";
	return 0;
}

