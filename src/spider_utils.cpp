#include <time.h>
#include <assert.h>

#include "spider_common.h"
#include "cryptopp/eccrypto.h"
#include "cryptopp/pkcspad.h"
#include "cryptopp/randpool.h"
#include "cryptopp/rsa.h"
#include "cryptopp/osrng.h"
#include "cryptopp/hex.h"
#include "cryptopp/base64.h"
using namespace CryptoPP;


#ifdef LINUX
#define min(a,b) (a)>(b)?(b):(a)
#define max(a,b) (a)>(b)?(a):(b)
#endif


int  init_network()
{
	int ret=0;
#ifdef WIN32
	WSAData	wsaData;
	memset(&wsaData, 0 , sizeof(WSAData));
	ret=WSAStartup(MAKEWORD(2,2),&wsaData);
#endif
	return ret;
}

void uninit_network()
{
#ifdef WIN32
	WSACleanup();
#endif
}

char dec_to_char(int n)
{
	if ( 0<=n&&n<=9 )
	{
		return char('0'+n);
	}
	else if ( 10<=n&&n<=15 )
	{
		return char('A'+n-10);	
	}
	else
	{
		return char(0);
	}
}

int char_to_dec(char c)
{
	if ( '0'<=c&&c<='9' )
	{
		return (int)(c-'0');
	}
	else if('a'<=c&&c<='f')
	{
		return (int)(c-'a'+10);
	}
	else if('A'<=c&&c<='F')
	{
		return (int)(c-'A'+10);
	}
	else
	{
		return -1;
	}
}

int hex_to_dec(char *source)  //16进制转换成10进制
{
	char *p = source;
	if(*p == '\0')
		return 0;

	while(*p == '0')
		p++;

	int dec = 0;
	char c;
	while(c = *p++)
	{
		dec <<= 4;
		int ret=char_to_dec(c);
		if ( ret!=-1 )
		{
			dec+=ret;
		}
		else
		{
			break;
		}
	}
	return dec;
}


std::string unicode_to_ansi(const char* unicode)
{
#ifdef LINUX 
	//TODO: 
#else
	std::string result;
	if ( unicode!=NULL )
	{
		int len=strlen(unicode)+1;
		char* outch=new char[len];
		wchar_t* w_char=new wchar_t[len];

		MultiByteToWideChar(CP_UTF8, 0, unicode, len, w_char, len);
		WideCharToMultiByte(CP_ACP, 0, w_char, len, outch , len, 0, 0);

		result.assign(outch);
		delete[] w_char;
		delete[] outch;
	}
	return result;
#endif
}

std::string get_date()
{
	time_t t = time(0); 
	char buffer[64]; 
	strftime( buffer, sizeof(buffer), "%Y%m%d",localtime(&t) ); 
	return std::string(buffer); 
}

/*
 求两个字符串的相似度
*/
float  get_similarity_degree(const char* source, const char* target)
{
	assert(source!=NULL&&target!=NULL);
	int source_length=strlen(source);
	int target_length=strlen(target);
	
	int  distance[100][100]={0};
	for (int i=0; i<=source_length;i++  )
	{
		distance[i][0]=0;
	}
	for (int i=0;i<=target_length;i++)
	{
		distance[0][i]=0;
	}
	
	for (int i=1; i<=source_length; i++  )
	{
		for ( int j=1; j<=target_length;j++ )
		{
			if ( source[i-1]!=target[j-1] )
			{
				int d1=distance[i-1][j]+1;  //delete source _i
				int d2=distance[i][j-1]+1;  //add source_i
				int d3=distance[i-1][j-1]+1; //replace source_i
				distance[i][j]=min(min(d1,d2), d3);
			}
			else
			{
				distance[i][j]=distance[i-1][j-1];
			}
		}
	}

	int ret_dis=distance[source_length][target_length];
	float result=1.0-((float)ret_dis)/(max(source_length, target_length)); 
	return result;
}


bool  is_url_char(char c)
{
	bool bret=false;
	char url[17]={'.', '_' , '\\', '/', '~', '%', '-' , '+', '&', '#', '?', '!', '=', '(', ')', '@', ':' };
	if ( (c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9') )
	{
		bret=true;
	}
	else
	{
		for (int i=0; i<17 ;i++)
		{
			if ( c==url[i])
			{
				bret=true;
				break;
			}
		}
	}
	return bret;
}

int get_href_img(const char* root_path_url, const char* current_path_url, const char* html, int pos, int is_href , char* child_url )
{
	if (  is_href==1 )
	{
		while(strncmp(html+pos,"href=\"", 6)!=0&&pos++);
		pos=pos+6;
	}
	else
	{
		while(strncmp(html+pos,"src=\"", 5)!=0&&pos++);
		pos=pos+5;		
	}

	int i=pos;

	while ( html[i]!='"'&&html[i]!='\0'&&i++ );

	if ( strncmp(html+pos, "javascript:;", 12) !=0 )
	{
		if ( strncmp(html+pos,"http://",7 )==0 )
		{
			strncpy(child_url,html+pos, i-pos);	
		}
		else if ( *(html+pos)=='/' )
		{
			strcpy(child_url, root_path_url );
			strncat(child_url, html+pos, i-pos );
		}
		else
		{
			strcpy(child_url,current_path_url);
			strncat(child_url, html+pos, i-pos);
		}
	}

	return i;
}


////////////////// 字符串查找KMP算法
void get_next(const char* partten, int* next)
{
	int i=1;
	int  k=0;
	while (partten[i]!='\0')
	{
		if ( partten[k]==partten[i] )
		{
			k++;
			next[i]=k;
		}
		else if ( partten[k]!=partten[i] )
		{
			k=0;
			next[i]=k;
		}
		i++;
	}
}

int  get_match(const char* source, int start, int end,const char* partten, int* next)
{
	int ret=0;
	int partten_length=strlen(partten);
	int comp_position=0;
	int result_pos=0;
	
	for ( int i=start; i<=end ;)
	{
		int j=comp_position;
		for ( ; j<partten_length ;j++ )
		{
			if ( source[i]==partten[j] )
			{
				i++;
			}
			else
			{
				if ( j>1 )
				{
					comp_position=next[j-1];
				}
				break;
			}
		}

		if ( j==partten_length )
		{
			ret=i;
			break;
		}
		i++;
	}
	return ret;
}

//
int  get_root_current_url(const char* url, char* root_path_url, char* current_path_url )
{
	int i=7;  //pos start from http:// 
	int root_pos=0;
	int current_pos=0;
	while (url[i]!='\0')
	{
		if ( url[i]=='/' )
		{
			if ( root_pos==0)
			{
				root_pos=i;
			}
			current_pos=i;
		}
		++i;
	}
	strncpy(root_path_url, url, root_pos );
	root_path_url[root_pos]='\0';
	strncpy(current_path_url, url, current_pos+1);
	current_path_url[current_pos+1]='\0';
	return 0;
}

// 对href标签的解析
int get_all_url(const char* url, const char* html, int start, int end,StrVec& url_array )
{
	const char* partten[2]={"<img ","<a "};
	const char* p=html;
	char root_path_url[1024];
	char current_path_url[1024];
	get_root_current_url(url, root_path_url, current_path_url);

	int length=strlen(html);
	for ( int k=0; k<2; k++ )  //对
	{
		int next[7]={0};
		get_next( partten[k] ,next);
		int  i=start;
		while (p[i]!='\0'&&i<end)
		{
			int pos=get_match(p, i, length , partten[k], next );
			if ( pos<=0 )
			{
				break;
			}
			else
			{
				char child_url[1024]={0};
				i=get_href_img(root_path_url,current_path_url, p,pos, k, child_url);
				if ( child_url[0]!='\0' )
				{
					url_array.push_back(std::string(child_url));
				}
			}
		}
	}

	return 0;
}


bool is_picture(const char* url)
{
	bool  bret=false;
	const char* pic_extension[]={".jpg", ".gif", ".png"};
	int i=0;
	int length=strlen(url);
	
	for ( int i=0; i<3; i++ )
	{
		if ( strcmp(url+length-4,pic_extension[i])==0 )
		{
			bret=true;
			break;
		}
	}
	return bret;
}

/////根据提供的range 返回要解析的网页区域
int  get_html_range(char* html, const char* mark, int& start, int& end)
{
	int ret=-1;
	if ( html!=NULL&&mark!=NULL)
	{
		int* next=new int[strlen(mark)];
		get_next(mark, next);
		start=get_match(html,0,strlen(html),mark,next);
		if ( start>0 )
		{
			char* pos=&html[start];
			int     quotes_count=0; // 引号计数
			int     tag_count=1;     //tag
			while ( pos!='\0' )
			{
				std::string node; //<dev
				if ( *pos=='<'&&quotes_count%2==0 )
				{
					if ( *++pos=='/' )
					{
						tag_count--;
					}
					else
					{
						tag_count++;
					}
				}
				else if ( *pos=='/'&&*++pos=='>'&&quotes_count%2==0 )
				{
					tag_count--;
				}
				else if( *pos=='\''||*pos=='\"' )
				{
					quotes_count++;	
				}
				
				if ( tag_count==0 )
				{
					end=pos-html;
					ret=0;				
					break;
				}
				pos++;
			}
		}
	}
	return ret;
}




std::string trim_string(const std::string& str, const std::string& drop)
{
	std::string r = str;
	// trim right
	r = r.erase(str.find_last_not_of(drop)+1);
	// trim left
	return r.erase(0,r.find_first_not_of(drop));	
}

int split_string(const std::string& str, StrVec& items, const std::string& splitter)
{
	std::string src_str = trim_string(str, splitter);
	int pos = 0;
	while (1)
	{
		if (src_str == splitter || src_str.empty())
		{
			break;
		}
		pos = src_str.find(splitter);
		items.push_back(src_str.substr(0, pos));
		if (pos == std::string::npos)
		{
			break;
		}
		src_str = src_str.substr(pos+splitter.size());
		src_str = trim_string(src_str, splitter);
	}
	return items.size();
}



int prase_url(const std::string & url, std::string & host, std::string & uri ,int & port)
{
	size_t host_begin = 7;
	size_t host_end = url.find_first_of(":/",host_begin);
	host = url.substr(host_begin,host_end-host_begin);
	if (host_end != std::string::npos)
	{
		if (url[host_end] == ':')
		{
			size_t port_end = url.find_first_of('/',host_end+1);
			std::string port_s = url.substr(host_end + 1, port_end - host_end);
			port = atoi(port_s.c_str());
			uri = url.substr(port_end);
		}
		else
		{
			port = 80;
			uri = url.substr(host_end);
		}
	}
	return 0;
}

int get_file_ext( const char* filename, char* ext )
{
	if( filename!=NULL&& ext!=NULL)
	{
		const char* p=filename+strlen(filename);
		while (*p!='.')p--;
		strcpy(ext, p);
	}
	return 0;
}

std::string base64_encrypt(const char* data,int data_length)
{
	int pos;
	std::string source;
	source.assign(data,data_length);
	std::string result;
	StringSource s(source, true, new Base64Encoder(new StringSink(result)) );
	if ( (pos=result.find('\n'))!=std::string::npos )
	{  //不知为啥有换行符
		result.erase(pos);
	}
	return result;
}


std::string RSA_encrypt(const char* data ,  const char* n, const char* e )
{  //crypto++ 采用MDD编译
	Integer cry_n(n);
	Integer cry_e(e);
	std::string source(data);
	RSAFunction params;
	params.Initialize(cry_n,cry_e);
	RSA::PublicKey pulic_key(params);

	AutoSeededRandomPool  rng;
	RSAES_PKCS1v15_Encryptor encoder(pulic_key);
	std::string result;
	StringSource(source,true,
		new PK_EncryptorFilter(rng,encoder,new HexEncoder(new StringSink(result)))
		);

	return result;
}


