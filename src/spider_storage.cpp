#include "spider_storage.h"
#include "spider_config.h"
#include "spider_url_rinse.h"
#include "spider_md5.h"

#include "boost/filesystem.hpp"

using namespace boost::filesystem;

Spider_Storage::Spider_Storage()
{
	
}

Spider_Storage::~Spider_Storage()
{
	
}

int Spider_Storage::initialize()
{
	m_database=new Spider_Database();
	m_database->initialize();
	
	return 0;
}

int Spider_Storage::uninitialize()
{
	m_database->uninitialize();
	if ( m_database!=NULL )
		delete m_database;

	return 0;
}

int Spider_Storage::write_file(const char* website, UrlPtr url_ptr )
{
	if ( website!=NULL&&url_ptr->filename!=NULL&&url_ptr->response!=NULL )
	{
		// 记录表中没有 可以入库
		if ( false==Spider_Url_Rinse::instance().search_and_record_history(url_ptr) )
		{
			boost::filesystem::path  file_path=Spider_Config::instance().storage_path_;

			bool  ret=true;
			if ( !exists(file_path) )
			{
				ret=create_directories(file_path);
			}
			if ( ret )
			{
				rename_filename_with_md5(url_ptr);
				file_path/=url_ptr->filename;
				if ( boost::filesystem::exists(file_path) )
				{
					LLOG(L_DEBUG, "%s is already exist.", file_path.string().c_str());
				}
				else
				{
					FILE* file=fopen(file_path.string().c_str(),"wb");
					if ( file!=NULL )
					{
						fwrite(url_ptr->response,1,url_ptr->length, file);
						fclose(file);
					}
				}
				m_database->insert_record(website,NULL, url_ptr);
			}
		}
	}
	return 0;
}

int Spider_Storage::rename_filename_with_md5( UrlPtr url_ptr )
{
	char fileext[10]={0};
	char filemd5[33]={0};
	Spider_MD5::get_file_md5(url_ptr->response, url_ptr->length, filemd5);
	get_file_ext(url_ptr->filename,fileext);
	char new_file_name[100];
	sprintf(new_file_name, "%s%s", filemd5, fileext);

	free(url_ptr->filename);
	url_ptr->filename=strdup(new_file_name);

	return 0;
}

