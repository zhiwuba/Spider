#include "spider_seed.h"
#include "spider_config.h"
#include "pugixml/pugixml.hpp"


void Seed::set_pic_size( const char* c )
{
	pic_size_.first=atoi(c)*1024;
	while(*c!='-'&&*c!='\0')c++;
	pic_size_.second=(*c=='-')?atoi(++c)*1024:0;
}



/////////////////////////////////////////////////
Spider_Seed::Spider_Seed()
{
	
}


Spider_Seed::~Spider_Seed()
{
	
}

int Spider_Seed::get_seed(std::string& website, Seed** seed )
{
	std::map<std::string,Seed*>::iterator iter=m_seeds.begin();
	if ( iter!=m_seeds.end() )
	{
		website=iter->first;
		*seed=iter->second;
		m_seeds.erase(iter);
	}
	return 0;
}

int Spider_Seed::load()
{
	std::string seed_path=Spider_Config::instance().module_path_+kSeedFileName;
	pugi::xml_document doc;

	pugi::xml_parse_result result=doc.load_file(seed_path.c_str());
	if(pugi::status_ok!=result.status )
	{
		LLOG(L_ERROR,"load seed file error.");
		return -1;
	}

	const char* query_path="/seeds/seed";
	pugi::xpath_node_set seed_set=doc.select_nodes(query_path);

	for ( pugi::xpath_node_set::const_iterator iter=seed_set.begin(); iter!=seed_set.end(); ++iter )
	{
		pugi::xml_node vnode=iter->node();
		if ( vnode )
		{
			std::string seed_name=vnode.attribute("name").as_string();
			std::string pic_size_str=vnode.attribute("pic_size").as_string();

			Seed* seed=new Seed;

			pugi::xpath_node_set start_urls=vnode.select_nodes("start/url");
			pugi::xpath_node_set pic_urls=vnode.select_nodes("pic/url");
			pugi::xpath_node_set index_urls=vnode.select_nodes("index/url");

			for ( pugi::xpath_node_set::const_iterator start_iter=start_urls.begin(); start_iter!=start_urls.end(); ++start_iter )
			{
				seed->start_url_.push_back(start_iter->node().text().as_string());
			}

			for ( pugi::xpath_node_set::const_iterator pic_iter=pic_urls.begin(); pic_iter!=pic_urls.end(); ++pic_iter )
			{
				seed->pic_regex_.push_back(boost::xpressive::cregex::compile( pic_iter->node().text().as_string() ) );
			}

			for ( pugi::xpath_node_set::const_iterator index_iter=index_urls.begin(); index_iter!=index_urls.end(); ++index_iter )
			{
				Seed::Index_Regex index_regex;
				pugi::xml_attribute com_attr=index_iter->node().attribute("comment");
				if( com_attr.empty() )
				{
					index_regex.has_comment=false;
					index_regex.index_regex=boost::xpressive::cregex::compile(index_iter->node().text().as_string());
				}
				else
				{
					index_regex.has_comment=true;
					index_regex.index_regex=boost::xpressive::cregex::compile(index_iter->node().text().as_string());
					index_regex.comment_regex=boost::xpressive::cregex::compile(com_attr.as_string());
				}
				seed->index_regex_.push_back(index_regex);
			}
	
			seed->set_pic_size(pic_size_str.c_str());
			m_seeds[seed_name]=seed; //´æ´¢
		}
	}

	return 0;
}
