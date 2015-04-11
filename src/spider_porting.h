#ifndef  __CROTON_PORTING_H__ 
#define __CROTON_PORTING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <time.h>

#include <string>
#include <map>
#include <list>
#include <vector>
#include <queue>



#ifdef WIN32
#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <time.h>
#include <Iphlpapi.h>
#include <Ws2tcpip.h>
#include <direct.h>
#include <io.h>
#else
#include <stdarg.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/param.h> 
#include <sys/ioctl.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#endif


typedef  unsigned long  ulong;

#ifdef WIN32
#define  lasterror WSAGetLastError()
typedef int  socklen_t;
typedef HANDLE handle_thread;
typedef HANDLE handle_mutex;
typedef HANDLE handle_semaphore;
typedef CRITICAL_SECTION* handle_recursivemutex;
typedef unsigned ( __stdcall *THREAD_FUN)( void * );
#define access  _access
#define mkdir   _mkdir
#define PATH_SEPARATOR  '\\'
#else
#define lasterror errno
typedef  int  SOCKET;
typedef pthread_t* handle_thread;
typedef pthread_mutex_t* handle_mutex;
typedef sem_t* handle_semaphore;
typedef pthread_mutex_t* handle_recursivemutex;
typedef void* (*THREAD_FUN)( void * );
#define INFINITE 0
#define PATH_SEPARATOR  '/'
#define stricmp strcasecmp
#endif


#ifndef EWOULDBLOCK		// Berkely compatiable.
#define EWOULDBLOCK WSAEWOULDBLOCK
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT WSAETIMEDOUT
#endif

#ifndef ENETRESET
#define ENETRESET WSAENETRESET
#endif

#ifndef EINPROGRESS
#define EINPROGRESS WSAEINPROGRESS
#endif

/*日志等级*/
enum LOG_LEVEL
{
	L_DEBUG,
	L_TRACE,
	L_WARN,
	L_ERROR
};
void          gettime(char* sztime);
extern LOG_LEVEL  g_LogLevel;  //日志等级
#define LLOG(level,fmt,...)  \
do\
{\
if (level>=g_LogLevel)\
{\
char local_time[32]={0};\
gettime(local_time);\
printf("%s", local_time);\
printf(fmt , ##__VA_ARGS__);\
printf("\n");\
}\
} while(0)

#define LLOG2(level,fmt,...)  \
do\
{\
if (level>=g_LogLevel)\
{\
	printf("[%s|%s|%d]",__FILE__,__FUNCTION__,__LINE__); \
	printf(fmt , ##__VA_ARGS__);\
	printf("\n");\
}\
} while(0)

#ifdef  LINUX
ulong GetTickCount();
void   Sleep(ulong millisecond);
int closesocket(int sock);
#endif


int            SetSockNoblock(int sock, int mode);
std::string GetMacAddress();
std::string GetExePath();
int            create_dir(const char* dir);

/* 多线程的封装 */
handle_thread thread_create(void* security, unsigned stack_size, THREAD_FUN start_address, 
							void* arglist, unsigned initflag /*= 0*/, unsigned* thraddr /*= NULL*/);
int thread_waitforend(handle_thread hThread, unsigned long dwMilliseconds);
bool thread_close(handle_thread hThread);
void thread_end(unsigned retval);

handle_mutex mutex_create();
bool mutex_destroy(handle_mutex handle);
bool mutex_lock(handle_mutex handle);
bool mutex_unlock(handle_mutex handle);

handle_recursivemutex recursivemutex_create();
void recursivemutex_destory(handle_recursivemutex handle);
void recursivemutex_lock(handle_recursivemutex handle);
void recursivemutex_unlock(handle_recursivemutex handle);

handle_semaphore semaphore_create(long init_count, long max_count);
void  semaphore_destory(handle_semaphore handle);
bool  semaphore_wait(handle_semaphore handle);
bool  semaphore_release(handle_semaphore handle);


#endif
