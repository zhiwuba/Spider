#include "spider_url.h"
#include "spider_utils.h"
#include "spider_common.h"
#include "spider_md5.h"
#define NEW_METHOD 1

std::string url_encode(std::string source_url)
{
	std::string result;
	for ( unsigned int i=0; i<source_url.size(); i++ )
	{
		char c=source_url[i];
		if ( ('0'<=c&&c<='9')||('a'<=c&&c<='z')||('A'<=c&&c<='Z')||c=='.')
		{
			result+=c;
		}
		else
		{
			int j=(int)c;
			if ( j<0 )
			{
				j+=256;
			}
			result+='%';
			result+=dec_to_char((j>>4));
			result+=dec_to_char((j%16));
		}
	}
	return result;
}

std::string url_decode(std::string source_url)
{
	std::string result;
	for ( unsigned int i=0;i<source_url.size(); i++ )
	{
		char c=source_url[i];
		if ( c!='%' )
		{
			result+=c;
		}
		else
		{
			int first=(char_to_dec(source_url[++i])<<4);
			int second=char_to_dec(source_url[++i]);
			int num=first+second;
			result+=(char)num;
		}
	}
	return result;
}

const char*  get_filename_from_uri(const char* uri)
{
	bool  found=false;
	const char* c=uri+strlen(uri);
	while ( *c!='/'&&c>uri)
	{
		if ( *c=='.' )
			found=true;
		c--;
	}
	if ( found==true&&*c=='/' )
	{
		return ++c;
	}
	return "";
}

UrlPtr create_url(std::string url, URLTYPE  type)
{
	assert(!url.empty());
	std::string host, uri;
	int port=80;
	prase_url(url,host,uri, port);
	UrlPtr object(new URL());
	object->url=strdup(url.c_str());
	object->domain=strdup(host.c_str());
	object->res=strdup(uri.c_str());
	object->filename=strdup(get_filename_from_uri(uri.c_str()));
	object->port=port;
	object->type=type;
	object->comment=strdup("");
	return object;
}


#if NEW_METHOD
unsigned int url_hash_code(UrlPtr url)
{
	uint md5_code=Spider_MD5::get_buffer_md5_code(url->url, strlen(url->url));
	return md5_code;
}
#else
unsigned int url_hash_code(UrlPtr url)
{
	unsigned int ret=0;
	if (url!=NULL)
	{
		unsigned int h=url->port;
		unsigned int i=0;
		while (url->domain[i] != 0)
		{
			h = 31*h + url->domain[i];
			i++;
		}
		i=0;
		while (url->res[i] != 0)
		{
			h = 31*h + url->res[i];
			i++;
		}
		ret=h%kUrlHashSize;
	}
	return ret;
}
#endif


