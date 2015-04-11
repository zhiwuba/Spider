#include "spider_http_client.h"
#include "spider_utils.h"
#include "spider_cookie.h"

#define  SEND_BUFFER_SIZE  100*1024
#define  RECV_BUFFER_SIZE  10*1024

/////////////////////////////////////////////////////
void Http_Request_Package::set_request_line(std::string line)
{
	m_line=line;
}

void Http_Request_Package::set_field(std::string key, std::string value)
{
	m_fields[key]=value;
}
void Http_Request_Package::set_field(std::string key, int value)
{
	char buffer[20]={0};
	sprintf(buffer,"%d", value);
	m_fields[key]=std::string(buffer);
}

std::string Http_Request_Package::build_package()
{
	std::string package;
	package=m_line+"\r\n";
	FieldsMap::iterator iter=m_fields.begin();
	for ( ; iter!=m_fields.end() ;iter++ )
	{
		package+=iter->first;
		package+=": ";
		package+=iter->second;
		package+="\r\n";
	}
	package+="\r\n";
	package+=m_body;
	return package;
}

void Http_Request_Package::clear()
{
	m_fields.clear();
	m_line.clear();
	m_body.clear();
}


//////////////////////////////////////////////////////////////
int  Http_Response_Package::get_status_code()
{
	int code=-1;
	if ( m_line.find("HTTP")!=std::string::npos )
	{
		int begin=m_line.find_first_of(' ');
		int end=m_line.find_last_of(' ');
		std::string temp=m_line.substr(begin,end-begin);
		code=atoi(temp.c_str());
	}
	return code;
}

std::string  Http_Response_Package::get_field(std::string key)
{
	FieldsMap::iterator iter=m_fields.find(key);
	if ( iter!=m_fields.end() )
	{
		return iter->second;
	}
	else
	{
		return "";
	}
}

int Http_Response_Package::prase_package(std::string package)
{
	m_line.clear();
	m_fields.clear();
	StrVec fields;
	split_string(package,fields,"\r\n");
	if (fields.size()>1)
	{
		m_line=fields[0];
	}

	for (unsigned int i=1;i<fields.size(); i++ )
	{
		int colon_pos=fields[i].find(':');
		std::string key=fields[i].substr(0,colon_pos);
		std::string value=fields[i].substr(colon_pos+1);
		value=trim_string(value," ");
		if ( m_fields.find(key)==m_fields.end() )
		{
			m_fields[key]=value;
		}
		else
		{
			m_fields[key]+=";"+value;
		}
	}
	return 0;
}

void Http_Response_Package::set_body(char*& body,int length)
{
	m_length=length;
	m_body=body;
}

int  Http_Response_Package::get_body(char*& body)
{
	body=m_body;
	return m_length;
}

void Http_Response_Package::clear()
{
	m_length=0;
	m_line.clear();
	m_fields.clear();
	if ( m_body!=NULL )
	{
		free(m_body);
		m_body=NULL;
	}
}

///////////////////////////////////////////////////////
Spider_Http_Client_Base::Spider_Http_Client_Base(void)
{
    
}

Spider_Http_Client_Base::~Spider_Http_Client_Base(void)
{
    
}

int Spider_Http_Client_Base::connect_host(std::string host)
{
	int sock=socket(AF_INET,SOCK_STREAM,0);
	if (sock==-1 )
	{
		printf("connect_host: create socket error %d.",lasterror);
		return -1;
	}
	int port=80;
	int pos=host.find(':');
	if ( pos!=std::string::npos )
	{
		port=atoi(host.substr(pos+1).c_str());
		host=host.substr(0,pos);
	}

	struct sockaddr_in addr;
	struct addrinfo    *result;
	struct addrinfo    hints;
	int error;

	memset(&hints, 0, sizeof(hints));
	memset(&addr, 0, sizeof(addr));

	hints.ai_family = AF_INET;

	error = getaddrinfo(host.c_str(), NULL, &hints, &result);
	if (error != 0)
	{
		printf("illegal host name or ip. \n");
		closesocket(sock);
		return -1;
	}

	addr = *(struct sockaddr_in *) result->ai_addr;
	addr.sin_port = htons(port);
	freeaddrinfo(result);
	
	int ret=connect(sock,(const sockaddr*)&addr, sizeof(addr) );
	if ( ret!=0 )
	{
		LLOG(L_ERROR,"connect  %s  error, code:%d.", host.c_str(), lasterror);
		closesocket(sock);
		return -1;
	}
	return sock;
}

int Spider_Http_Client_Base::connect_ip(const char *ip, int port)
{
    int sock=socket(AF_INET,SOCK_STREAM,0);
    if (sock==-1 )
    {
		printf("create socket error,code :%d\n",lasterror);
        return -1;
    }
    
    struct sockaddr_in serveraddr;
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_port=htons(port);
    serveraddr.sin_addr.s_addr=inet_addr(ip);
    
    int ret=connect(sock,(struct sockaddr*)&serveraddr ,sizeof(serveraddr) );
    if ( ret!=0 )
    {
		printf("connect %s  error. code:%d.\n", ip,lasterror );
		closesocket(sock);
        return -1;
    }
	

	//linger m_sLinger;
	//m_sLinger.l_onoff = 1;	// (在closesocket()调用,但是还有数据没发送完毕的时候容许逗留)
	//m_sLinger.l_linger = 0;	// (容许逗留的时间为0秒)
	//setsockopt(sock,SOL_SOCKET,SO_LINGER, (const char*)&m_sLinger,sizeof(linger));

	//struct timeval tv;
	//tv.tv_sec=20;
	//tv.tv_usec=0;
	//setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(const char*)&tv, sizeof(tv));
	//setsockopt(sock,SOL_SOCKET,SO_SNDTIMEO,(const char*)&tv, sizeof(tv));

    return sock;
}


int Spider_Http_Client_Base::send_package(int sock, Http_Request_Package& request )
{
    if (sock<=0) {
        printf("send_package sock is invalid. \n");
        return -1;
    }
    
	std::string package=request.build_package();
    const char* buffer=package.c_str();

	int length=package.size();
    int  left=length;
    while (left>0)
    {
        int send_count=left>SEND_BUFFER_SIZE?SEND_BUFFER_SIZE:left;
        int ret=send(sock, buffer+length-left, send_count, 0);
        if (ret<0)
        {
			int last_error=lasterror;
			if ( last_error>0&&last_error!=EWOULDBLOCK&&last_error!=EINPROGRESS )
			{
				printf("send_package:send error, code:%d.\n",last_error);
				break;
			}
        }
        else if (ret==0)
        {
            if (errno==EAGAIN)
                continue;
        }
        else
        {
            left-=ret;
        }
    }

	return left==0?0:-1;
}


int Spider_Http_Client_Base::recv_package(int sock, Http_Response_Package& response)
{
	char recvbuffer[4096]={0};
	int count=recv_header(sock,recvbuffer);
	if ( count<0 )
	{
		LLOG(L_ERROR,"recv_header error.");
		return -1;
	}

	response.prase_package(recvbuffer);
	std::string result=response.get_field("Transfer-Encoding");
	if( result.compare("chunked")==0)
	{   //chunk的下载方式
		char* body=NULL;
		int length=recv_body_bychunk(sock,body);
		response.set_body(body,length);
	}
	else
	{   //Content-Length的下载方式
		int length=atoi(response.get_field("Content-Length").c_str());
		if ( length>0 )
		{
			char* body=(char*)malloc(length);
			count=recv_body_bylength(sock,body,length);
			if (count!=length)
			{
				LLOG(L_ERROR,"recv_package:recv_body_bylength error.");
				return -1;
			}
			response.set_body(body, length);
		}
	}
	return 0;
}

int Spider_Http_Client_Base::recv_header(int sock, char*content)
{
	return recv_endwith_mark(sock,content,"\r\n\r\n");
}

int Spider_Http_Client_Base::recv_body_bylength(int sock, char*body, int length)
{
    if (sock<=0)
    {
        printf("recv_body_bylength: sock is invalid. \n");
        return -1;
    }
    int left=length;
    while (left>0)
    {
        int ret=recv(sock, body+length-left, left, 0);
        if (ret<0)
		{
			int last_error=lasterror;
			if ( last_error>0&&last_error!=EWOULDBLOCK&&last_error!=EINPROGRESS )
			{
				printf("recv error %d.", last_error);
				break;
			}
        }
        else if (ret==0) {
            if (errno==EAGAIN) {
                continue;
            }
        }else {
            left-=ret;
        }
    }
    return length-left;
}

int Spider_Http_Client_Base::recv_body_bychunk(int sock, char*& body )
{
	int total_length=0;
	while (true)
	{
		int length=0;
		char buffer[48]={0};
		int ret=recv_endwith_mark(sock, buffer, "\r\n");
		if ( ret>48||ret<2 )
		{
			return -1;
		}
		buffer[ret-2]='\0';
		length=hex_to_dec(buffer);
		if ( length<=0 )
		{
			break;
		}
		total_length+=length;
		body=(char*)realloc(body,total_length);
		ret=recv_body_bylength(sock,body+total_length-length,length);
		if ( ret !=length)
		{
			LLOG(L_ERROR,"recv_body_bychunk::ret isn't equal to length. ");
			break;
		}
		ret=recv_endwith_mark(sock,buffer,"\r\n");
		if ( ret!=2 )
		{
			LLOG(L_ERROR,"recv_endwith_mark:: ret isn't 2");
			break;
		}
	}
	return total_length;
}

int Spider_Http_Client_Base::recv_endwith_mark(int sock, char* buffer,const char* mark)
{
	if (sock<=0)
	{
		printf("recv_endwith_mark: sock is invalid. \n");
		return -1;
	}

	int count=0;
	while (true) 
	{
		int ret=recv(sock, buffer+count, 1, 0);
		if (ret<0)
		{
			int last_error=lasterror;
			if ( last_error>0&&last_error!=EWOULDBLOCK&&last_error!=EINPROGRESS )
			{
				printf("recv_endwith_mark error,code : %d .\n", last_error);
				break;
			}
		}
		else if (ret==0)
		{
			if (errno==EAGAIN)
			{
				continue;
			}
		}
		else
		{
			count+=ret;
			if(NULL!=strstr(buffer, mark))
			{
				break;
			}
		}
	}
	buffer[count]='\0';
	return count;
}

int Spider_Http_Client_Base::close_socket(int sock)
{
    if (sock<0) {
        return -1;
    }
	//shutdown(sock,SD_BOTH);
    closesocket(sock);
    sock=0;
    return 0;
}

int Spider_Http_Client_Base::surface_page(std::string host,Http_Request_Package& request, Http_Response_Package& response)
{
	int sock=connect_host(host);
	if ( sock<=0 )
	{
        LLOG(L_ERROR, "surface_page connect_host error.");
		return -1;
	}

	int ret=send_package(sock,request);
	if ( ret!=0 )
	{
		LLOG(L_ERROR,"surface_page:send_package error. ");
		return -1;
	}
	
	ret=recv_package(sock, response);
	if (ret!=0)
	{
		LLOG(L_ERROR,"surface_page:recv_package error. ");
		return -1;
	}

	int status_code=response.get_status_code();
	close_socket(sock);
	return status_code;
}


////////////////////////////////////////////////////////////////////
Spider_Http_Client::Spider_Http_Client()
{

}

Spider_Http_Client::~Spider_Http_Client()
{

}

int  Spider_Http_Client::send_request(UrlPtr url)
{
	if (url==NULL||url->ip==NULL)
	{
		LLOG2(L_ERROR,"send_request: url is invalid.");
		return -1;
	}
	int sock=connect_ip(url->ip, url->port);
	if ( sock<=0 )
	{
		LLOG2(L_ERROR,"send_request:failed  to connect.");
		return -1;
	}
	Http_Request_Package  request_package;
	std::string request_line="GET ";
	request_line+=url->res;
	request_line+=" HTTP/1.1";
	request_package.set_request_line(request_line);
	request_package.set_field("Accept","*/*");
	request_package.set_field("User-Agent","Mozilla/5.0");
	request_package.set_field("Host", std::string(url->domain));
	request_package.set_field("Connection","close");  //短链接
	if ( strstr(url->domain,"renren")!=NULL )
	{
		Cookie* cookie=Spider_Cookie::instance().get_cookie("renren");
		if ( cookie!=NULL )
		{
			request_package.set_field("Cookie",cookie->to_string() );
		}
	}
	else if(strstr(url->domain,"weibo")!=NULL )
	{
		Cookie* cookie=Spider_Cookie::instance().get_cookie("weibo");
		if ( cookie!=NULL )
		{
			request_package.set_field("Cookie",cookie->to_string() );
		}
	}

	int ret=send_package(sock, request_package);
	if ( ret!=0 )
	{
		LLOG2(L_WARN,"send count isn't equal to ret. Why???");
	}

	return sock;
}

int Spider_Http_Client::recv_response(int sock , std::string& body )
{
	Http_Response_Package response_package;
	int ret=recv_package(sock, response_package);
	if (ret!=0)
	{
		LLOG2(L_ERROR,"recv occur an error. ");
	}
	close_socket(sock); //一定要关闭socket幺....

	int status_code=response_package.get_status_code();
	if (status_code==200)
	{
		char* buffer=NULL;
		int length=response_package.get_body(buffer);
		body.assign(buffer, length);
	}
	else if (status_code==302)
	{
		body=response_package.get_field("Location");
	}

	return status_code;
}

int Spider_Http_Client::recv_response(int sock , char** body, int& length  )
{
	Http_Response_Package response_package;
	int ret=recv_package(sock, response_package);
	if (ret!=0)
	{
		close_socket(sock);
		LLOG2(L_ERROR,"recv occur an error. ");
		return -1;
	}
	close_socket(sock); //一定要关闭socket幺....

	int status_code=response_package.get_status_code();
	char* buffer=NULL;
	length=response_package.get_body(buffer);
	if ( length>0 )
	{
		*body=(char*)malloc(length);
		if( *body==NULL )
		{
			LLOG2(L_ERROR, "recv_response malloc error.");
			return -1;
		}
		memcpy(*body,buffer,length);
	}
	return  status_code;
}

