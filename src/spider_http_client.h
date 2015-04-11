#ifndef  __CROTON_SPIDER_HTTPOPERATOR_H__
#define __CROTON_SPIDER_HTTPOPERATOR_H__
#include "spider_common.h"
#include "spider_url.h"

class Http_Request_Package
{
public:
	Http_Request_Package()
	{
	};
	~Http_Request_Package()
	{
	};
	void set_body(std::string body){m_body=body;};  /*body内容少，不适合发大文件*/
	void set_request_line(std::string line);
	void set_field(std::string key, std::string value);
	void set_field(std::string key, int value);
	std::string build_package();  /*创建报文*/
	void clear();
private:
	std::string   m_line;  
	FieldsMap  m_fields;	
	std::string   m_body;
};

class Http_Response_Package
{
public:
	Http_Response_Package()
	{
		m_length=0;
		m_body=NULL;
	};
	~Http_Response_Package()
	{
		if ( m_body!=NULL )
		{
			free(m_body);
			m_body=NULL;
		}
	};

	int   prase_package(std::string package); /*解析报文*/
	int   get_status_code();  //返回状态码
	std::string  get_field(std::string key);
	void set_body(char*& body,int length);
	int   get_body(char*& body);
	void clear();
private:
	std::string   m_line;
	FieldsMap  m_fields;	
	char*          m_body;
	int              m_length;
};




class Spider_Http_Client_Base
{
public:
	Spider_Http_Client_Base(void);
	~Spider_Http_Client_Base(void);

	int surface_page(std::string host,Http_Request_Package& request, Http_Response_Package& response);


	int connect_host(std::string host);
	int connect_ip(const char* ip, int port);
	int send_package(int sock, Http_Request_Package& request);
	int recv_package(int sock, Http_Response_Package& response);
	int close_socket(int sock);

private:	
	int recv_endwith_mark(int sock, char* buffer,const char* mark);
	int recv_header(int sock, char* content);
	int recv_body_bylength(int sock, char* body, int length);
	int recv_body_bychunk(int sock, char*& body );

};

class Spider_Http_Client:public Spider_Http_Client_Base
{
public:
	Spider_Http_Client();
	~Spider_Http_Client();
	
	int send_request(UrlPtr  url);  /*创建socket 建立请求 返回socket*/
	int recv_response(int sock , std::string& body);             /* 接收所有响应 关闭socket*/
	int recv_response(int sock , char** body, int& length);

};


#endif
