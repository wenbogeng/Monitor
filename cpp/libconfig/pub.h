#pragma once


#if defined(WIN32)
#include <Windows.h>
#include <WinSock2.h>
#include<ws2tcpip.h>
#include <io.h>

#pragma comment(lib, "Ws2_32.lib")

#else                                                                                                    

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>                                                                                                                                                                                                                              
#endif


#if defined(WIN32)

#define CLOSESOCKET closesocket
#define IOCTLSOCKET ioctlsocket
 
#define PID  DWORD
#define SLEEP       Sleep
#define ACCESS      _access
#define POPEN       _popen
#define THREAD_HANDLE HANFLE
#else

#define THREAD_HANDLE pthread_t 

#define CLOSESOCKET close
#define IOCTLSOCKET ioctl
#define SLEEP       sleep
#define ACCESS      access
#define POPEN       popen

#define SOCKET int32_t
#define PID pthread_t

#endif


#include <stdio.h>
#include <stdlib.h>
#include <string.h>



typedef void* (*THREAD_FUNC)(void*);

#define MSG_MAX_LENGTH 2048

#define HEART_BEAT_PACKET_TYPE	    1  // heart beat testing packet
#define LOGON_PACKET_TYPE	    2  // online greeting packet
#define ASK_FOR_REPLY_PACKET_TYPE   3  // ask for cmd reply data packet, such as Transfer, LoadModules etc.
#define CMD_REPLY_PACKET_TYPE	    4  // one cmd  data packet
#define CMDS_REPLY_PACKET_TYPE	    5  // cmds  data packet
#define OTHER_NO_REPLY_PACKET_TYPE  6  // normal data packet

#define FILE_TRANSFER "File_Transfer" 
#define TRADE_MONITOR "Trade_Monitor"


struct MSG 
{
    int _type;
    char _content[MSG_MAX_LENGTH];
 
    MSG(): _type(OTHER_NO_REPLY_PACKET_TYPE) 
    {
        memset(_content, 0, sizeof(_content));
    }
};  

enum LogLevelType
{
    TRACE,
    DEBUG_MONITOR, // 重命名了 DEBUG 我的系统或库可能已经定义了 DEBUG，导致冲突
    INFO,
    WARNING,
    ERROR,
    FATAL
};


#define LOG_TRACE(fmt, ...)  log_trace(TRACE, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  log_debug(DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)   log_info(INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   log_warn(WARNING, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)  log_error(ERROR, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)  log_fatal(FATAL, fmt, ##__VA_ARGS__)


#define LOG_OUTPUT(fmt, ...)  \
    do {     \
	printf(fmt"\n", ##__VA_ARGS__);  \
	fflush(stdout);    \
    } while (0)


#define log_trace(level, format, ...) \
    do {\
	if (level >= TRACE) \
	  LOG_OUTPUT("%s TRACE " format, __TIME__, \
	              ##__VA_ARGS__);  \
    } while (0)


#define log_debug(level, format, ...) \
    do {\
	if (level >= DEBUG) \
	  LOG_OUTPUT("%s DEBUG_MONITOR " format, __TIME__, \
	              ##__VA_ARGS__);  \
    } while (0)


#define log_info(level, format, ...) \
    do {\
	if (level >= INFO) {\
	  LOG_OUTPUT("%s INFO " format, __TIME__, \
		      ##__VA_ARGS__);  \
	 }  \
    } while (0)



#define log_warn(level, format, ...) \
    do {\
	if (level >= WARNING) \
	  LOG_OUTPUT("%s WARNING FILE:%s LINE:%d " format, __TIME__, \
		      __FILE__, __LINE__, ##__VA_ARGS__);  \
    } while (0)



#define log_error(level, format, ...) \
    do {\
	if (level >= ERROR) \
	  LOG_OUTPUT("%s ERROR FILE:%s LINE:%d " format, __TIME__, \
		      __FILE__, __LINE__, ##__VA_ARGS__);  \
    } while (0)


#define log_fatal(level, format, ...) \
    do {\
	if (level >= FATAL) \
	  LOG_OUTPUT("%s FATAL FILE:%s LINE:%d " format, __TIME__, \
		      __FILE__, __LINE__, ##__VA_ARGS__);  \
    } while (0)
