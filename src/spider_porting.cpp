#include "spider_porting.h"

#ifdef _DEBUG
LOG_LEVEL g_LogLevel=L_DEBUG;
#else
LOG_LEVEL g_LogLevel=L_WARN;
#endif


#ifdef WIN32
//thread function.
handle_thread thread_create(void* security, unsigned stack_size, THREAD_FUN start_address, void* arglist, unsigned initflag /*= 0*/, unsigned* thraddr /*= NULL*/)
{
	return (handle_thread)_beginthreadex(security, stack_size, start_address, arglist, initflag, thraddr);
}

void thread_end(unsigned retval)
{
	_endthreadex(retval);
}

int thread_waitforend(handle_thread hThread, unsigned long dwMilliseconds)
{
	DWORD dwWaitResult = WaitForSingleObject(hThread, dwMilliseconds);
	switch (dwWaitResult) 
	{
	case WAIT_OBJECT_0: 
		return 0;
	case WAIT_TIMEOUT: 
	case WAIT_ABANDONED: 
	default:
		return -1;
	}

}

bool thread_close(handle_thread hThread)
{
	if (hThread != NULL)
	{
		if (CloseHandle(hThread))
			return true;
		else
			return false;
	}
	return true;
}

std::string GetMacAddress()
{
	std::string macaddr = "";

	IP_ADAPTER_INFO AdapterInfo[20];       // up to 20 NICs...
	DWORD dwBufLen = sizeof(AdapterInfo);

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if(dwStatus == ERROR_SUCCESS)
	{
		PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;

		while(pAdapterInfo) 
		{
			if(pAdapterInfo->Type == MIB_IF_TYPE_LOOPBACK)
			{
				pAdapterInfo = pAdapterInfo->Next;
				continue;
			}

			if(strstr(pAdapterInfo->Description, "VMware") > 0
				|| strstr(pAdapterInfo->Description, "Loopback")> 0
				|| strstr(pAdapterInfo->Description, "Bluetooth")> 0
				|| strstr(pAdapterInfo->Description, "Virtual")> 0
				)
			{
				pAdapterInfo = pAdapterInfo->Next;
				continue;
			}

			char temp[10] = {0};
			for(int i = 0 ; i < 6 ; i++ )
			{
				macaddr += _itoa((int)pAdapterInfo->Address[i], temp, 16);
			}
			break;
		}
	}
	return macaddr;
}

handle_mutex mutex_create()
{
	return CreateMutex( NULL, FALSE, NULL);
}

bool mutex_destroy(handle_mutex handle)
{
	if (handle == NULL)
	{
		return true;
	}
	if (CloseHandle(handle))
	{
		return true;
	}
	return false;
}

bool mutex_lock(handle_mutex handle)
{
	bool bres = false;
	if (handle == NULL)
	{
		return true;
	}

	DWORD dwWaitResult = WaitForSingleObject( 
		handle,   // handle to mutex
		INFINITE);   // five-second time-out interval

	switch (dwWaitResult) 
	{
		// The thread got mutex ownership.
	case WAIT_OBJECT_0: 
		bres = true;
		break;
		// Cannot get mutex ownership due to time-out.
	case WAIT_TIMEOUT: 
		// Got ownership of the abandoned mutex object.
	case WAIT_ABANDONED: 
		break;
	}

	return bres;
}

bool mutex_unlock(handle_mutex handle)
{
	if (handle == NULL)
	{
		return true;
	}
	if (ReleaseMutex(handle))
	{
		handle=NULL;
		return true;
	}
	return false;
}

handle_recursivemutex recursivemutex_create()
{
	CRITICAL_SECTION* pSect=new CRITICAL_SECTION();
	InitializeCriticalSection(pSect);
	return pSect;
}

void recursivemutex_destory(handle_recursivemutex handle)
{
	DeleteCriticalSection(handle);
	if (handle!=NULL)
	{
		delete handle;
		handle=NULL;
	}
}

void recursivemutex_lock(handle_recursivemutex handle)
{
	EnterCriticalSection(handle);
}

void recursivemutex_unlock(handle_recursivemutex handle)
{
	LeaveCriticalSection(handle);
}

handle_semaphore semaphore_create(long init_count, long max_count)
{
	handle_semaphore sem=CreateSemaphore(NULL,init_count,max_count,NULL);
	return sem;
}

void  semaphore_destory(handle_semaphore handle)
{
	if (handle!=NULL)
	{
		CloseHandle(handle);
		handle=NULL;
	}
}

bool  semaphore_wait(handle_semaphore handle)
{
	bool bret=false;
	if ( handle!=NULL )
	{
		WaitForSingleObject(handle,INFINITE);
		bret=true;
	}
	return bret;
}

bool  semaphore_release(handle_semaphore handle)
{
	return ReleaseSemaphore(handle,1,NULL);
}


int  SetSockNoblock(int sock, int mode)
{
	u_long mm=mode;
	return ioctlsocket(sock,FIONBIO , &mm );
}

std::string GetExePath()
{
	char cstr_path[MAX_PATH+1] = {0};
	TCHAR path[MAX_PATH+1] = {0};
	GetModuleFileName(NULL, path, MAX_PATH);
	WideCharToMultiByte(CP_ACP,0,path,MAX_PATH,cstr_path, MAX_PATH, NULL, 0);
	std::string full_path = cstr_path;
	int pos=full_path.find_last_of(PATH_SEPARATOR);
	return full_path.substr(0, pos+1);
}


#else

//thread function.
handle_thread thread_create(void* security, unsigned stack_size, THREAD_FUN start_address, void* arglist, unsigned initflag /*= 0*/, unsigned* thraddr /*= NULL*/)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, stack_size); 

	handle_thread hThread = new (std::nothrow) pthread_t;
	if(0 == pthread_create(hThread, &attr, start_address, arglist))
	{
		return hThread;
	}
	else
	{
		delete hThread;
		return NULL;
	}
}

void thread_end(unsigned retval)
{
	pthread_exit(&retval);
}

int thread_waitforend(handle_thread hThread, unsigned long dwMilliseconds)
{
	if(hThread == NULL)
		return 0;

	return pthread_join(*hThread, NULL);
}

bool thread_close(handle_thread hThread)
{
	if (hThread != NULL)
	{
		delete hThread;
	}

	return true;
}

ulong GetTickCount()
{
	timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec*1000L+tv.tv_usec/1000L;
}

void Sleep(ulong millisecond)
{
	usleep(millisecond*1000);
}

int closesocket(int sock)
{
	return close(sock);
}

handle_mutex mutex_create()
{
	handle_mutex handle = new (std::nothrow) pthread_mutex_t;
	if (pthread_mutex_init(handle, NULL) != 0)
	{
		delete handle;
		return NULL;
	}

	return handle;
}

bool mutex_destroy(handle_mutex handle)
{
	if (handle == NULL)
	{
		return true;
	}

	int iRes = pthread_mutex_destroy(handle);
	delete handle;

	if (iRes != 0)
	{
		return false;
	}

	return true;
}

bool mutex_lock(handle_mutex handle)
{
	if (handle == NULL)
	{
		return true;
	}

	int iRes = pthread_mutex_lock(handle);

	if (iRes != 0)
	{
		return false;
	}

	return true;
}

bool mutex_unlock(handle_mutex handle)
{
	if (handle == NULL)
	{
		return true;
	}

	int iRes = pthread_mutex_unlock(handle);

	if (iRes != 0)
	{
		return false;
	}

	return true;
}

handle_recursivemutex recursivemutex_create()
{
	pthread_mutex_t* pMutex=new pthread_mutex_t();
	pthread_mutexattr_t attr;
	pthread_mutexattr_init(&attr);
	pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(pMutex, &attr);
	return pMutex;
}

void recursivemutex_destory(handle_recursivemutex handle)
{
	if (handle!=NULL)
	{
		pthread_mutex_destroy(handle);
		delete handle;
		handle=NULL;
	}
}

void recursivemutex_lock(handle_recursivemutex handle)
{
	if (handle!=NULL)
	{
		pthread_mutex_lock(handle);
	}
}

void recursivemutex_unlock(handle_recursivemutex handle)
{
	if (handle!=NULL)
	{
		pthread_mutex_unlock(handle);
	}
}

handle_semaphore semaphore_create(long init_count, long max_count)
{
	handle_semaphore sem=new sem_t;
	int ret=sem_init(sem, 0, init_count );
	if ( ret==0 )
	{
		return sem;
	}
	else
	{
		return NULL;
	}
}

void  semaphore_destory(handle_semaphore handle)
{
	if ( handle!=NULL )
	{
		int ret=sem_destroy(handle);
		if ( ret!=0 )
		{
			printf("semaphore_destory error. \n");
		}
		delete handle;
		handle=NULL;
	}
}

bool  semaphore_wait(handle_semaphore handle)
{
	int bret=false;
	int ret=sem_wait(handle);
	if ( ret==0 )
	{
		bret=true;
	}
	return bret;
}

bool  semaphore_release(handle_semaphore handle)
{
	int bret=false;
	int ret=sem_post(handle);
	if ( ret==0 )
	{
		bret=true;
	}
	return bret;
}


int  SetSockNoblock(int sock, int mode)
{
	int ret=0;
	int flags = fcntl(sock, F_GETFL, 0); 
	if ( mode==1 )
	{
		ret=fcntl(sock, F_SETFL, flags|O_NONBLOCK);
	}
	else
	{
		ret=fcntl(sock,F_SETFL, flags|~O_NONBLOCK);
	}
	return ret;
}

std::string GetExePath()
{
	std::string full_path;
	char pidexe[1024]={0};
	snprintf(pidexe, sizeof(pidexe), "/proc/%u/exe", getpid());
	int fd = open(pidexe, O_RDONLY);
	if(fd == -1)
		return full_path;

	char buf[1024]={0};
	if(readlink(pidexe,buf, 1024)!=-1) 
	{
		full_path = buf;
	}
	close(fd);
	
	int pos=full_path.find_last_of(PATH_SEPARATOR);
	return full_path.substr(0,pos+1);
}


#endif


int  create_dir(const char* szdir)
{   /**  so perfect!! **/
	std::string dir(szdir);
	std::string parent;
	int pos=dir.find_last_of(PATH_SEPARATOR);
	if ( (pos+1)!=dir.length())
	{
		parent=dir.substr(0, pos+1);
	}
	else
	{
		pos=dir.find_last_of(PATH_SEPARATOR,pos-1);
		parent=dir.substr(0, pos+1);
	}

	if ( access(dir.c_str(), 0)!=0 )
	{
		create_dir(parent.c_str());
#ifdef WIN32
		mkdir(dir.c_str());
#else 
		mkdir(dir.c_str(),'0755');
#endif
	}
	else
	{
		return 0;
	}
	return 0;
}


void gettime(char* sztime)
{
	time_t local_time = time(0);
	struct tm * newtime = localtime(&local_time);
	sprintf(sztime, "%.19s : ", asctime(newtime));
}
