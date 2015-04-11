#ifndef  __CROTON_SPIDER_UTILS_H__
#define __CROTON_SPIDER_UTILS_H__

#include "spider_common.h"

//初始化网络__win32
int     init_network();
void  uninit_network();

//进制转换
int     hex_to_dec(char *s);
char  dec_to_char(int n);
int     char_to_dec(char c);

//获取日期
std::string get_date();

//编码转换
std::string unicode_to_ansi(const char* unicode);


// 获取相似度
float  get_similarity_degree(const char* source, const char* target);

//匹配字符串 KMP
void  get_next(const char* partten, int* next);
int    get_match(const char* source, int start, int end,  const char* partten, int*next);

//摘取url
bool  is_url_char(char c);
int   get_all_url(const char* url, const char* html, int start, int end,StrVec& url_array);

//根据标签获取范围(利用对称性)
int  get_html_range(char* html, const char* mark, int& start, int& end);

std::string trim_string(const std::string& str, const std::string& drop);
int            split_string(const std::string& str, StrVec& items, const std::string& splitter);
int            prase_url(const std::string & url, std::string & host, std::string & uri ,int & port);

//加密
std::string base64_encrypt(const char* data,int data_length);
std::string RSA_encrypt(const char* data ,  const char* n, const char* e);

int get_file_ext(const char* filename, char* ext);

class Recursive_Lock
{
public:
	explicit Recursive_Lock(handle_recursivemutex hmutex):m_recursivemutex(hmutex)
	{
		recursivemutex_lock(m_recursivemutex);
	}

	~Recursive_Lock()
	{
		recursivemutex_unlock(m_recursivemutex);
	}
private:
	handle_recursivemutex m_recursivemutex;
};


#endif
