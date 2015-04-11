#ifndef  __CROTON_SPIDER_COMMON_H__
#define __CROTON_SPIDER_COMMON_H__
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <list>
#include <iostream>

#ifdef WIN32
#include <Winsock2.h>
typedef unsigned __int64  uint64_t;
#endif

#include "spider_porting.h"

typedef unsigned int  uint;
typedef unsigned char uchar;


static const  int   kDomainTableSize=100*1024;   //100K
static const  int   kDomainHashSize=20000;        //2W个域名
static const  int   kUrlTableSize=100*1024; 
static const  uint   kUrlHashSize=0xfffffffe;    //513M的过滤表

static const  int	kThreadPoolSize=20;   //开启20个线程池
static const  int   kProcessCountPer=20;  //每次处理
static const  int   kMinSet=5;               //还有5个的时候请求
static const  long kRecordHistoryInterval=1000*60; //unit: ms. 1分钟

static const  char*  kConfigFileName="spider.conf"; 
static const  char*  kSeedFileName="seed.xml";
static const  char*  KCookieFileName="spider.cookie";
static const  char*  kDnsFileName="spider.dns";
static const  char*  kHistoryFileName="spider.history";


typedef std::vector<std::pair<std::string,std::string > > StrPairVec;
typedef std::vector<std::string> StrVec;
typedef std::pair<int,int>           IntPair;
typedef std::map<std::string, std::string> FieldsMap;

#endif
