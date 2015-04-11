#ifndef   __CROTON_SPIDER_SEED_H__
#define   __CROTON_SPIDER_SEED_H__
#include "spider_utils.h"
#include "boost/xpressive/xpressive_dynamic.hpp"

class Seed
{
public:
	void set_pic_size(const char* str);

	struct Index_Regex
	{
		bool has_comment;
		boost::xpressive::cregex  index_regex;
		boost::xpressive::cregex  comment_regex;
	};

	StrVec  start_url_;                                    //爬取页面
	std::vector<Index_Regex> index_regex_;  //second存储content的正则
	std::vector<boost::xpressive::cregex> pic_regex_;
	IntPair  pic_size_;
};

class Spider_Seed
{
public:
	Spider_Seed();
	~Spider_Seed();
	
	int load();
	int get_seed(std::string& website, Seed** seed );
	
private:
	std::map<std::string,Seed*> m_seeds;
};


#endif

