#ifndef  __CROTON_SPIDER_URL_H__
#define __CROTON_SPIDER_URL_H__

#include "spider_common.h"
#include "boost/shared_ptr.hpp"


enum URLTYPE
{
	UT_START, //start
	UT_INDEX, //index
	UT_PICT,    //picture
};

class URL
{
public:
	URL()
	{
		parent=NULL;
		url=NULL;
		domain=NULL;
		res=NULL;
		ip=NULL;
		response=NULL;
		belong=NULL;
		length=0;
		comment=NULL;
		albums_id=0;
	}
	~URL()
	{
		if (parent!=NULL)free(parent);
		if (url!=NULL) free(url);
		if (domain!=NULL)free(domain);
		if (res!=NULL )free(res);
		if (ip!=NULL)  free(ip);
		if (response!=NULL)free(response);
		if (comment!=NULL)free(comment);
	}
	
	char*  parent;    //parent url for pic
	char*  url;          //url 
	char*  domain;  //domain
	char*  res;         //Request Resource 
	char*  ip;           //ip 
	int      port;       //port default  80
	char*  filename; //file name
	int      albums_id; //the id of albums which this url belong.
	char*  response;  //http response
	int      length;      //the length of response.
	void*  belong ;    //the website which url belong.
	URLTYPE type;    //whether picture or html.
	char* comment;  //comment of gifs.
};

typedef  boost::shared_ptr<URL>   UrlPtr; 
typedef  std::vector<UrlPtr>           UrlPtrVec;

std::string url_encode(std::string source_url);
std::string url_decode(std::string source_url);
UrlPtr       create_url(std::string url, URLTYPE  type);

unsigned int url_hash_code(UrlPtr url);

#endif