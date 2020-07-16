#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <process.h>
#else
#ifdef _IOS
#include <dispatch/dispatch.h>
#include <mach/mach_time.h>
#else
#include <semaphore.h>
#endif
#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#endif

#include <sstream>
#include <stdlib.h>
#include <stdint.h>

using namespace std;

namespace XMemory
{
#ifdef _WIN32
	typedef CRITICAL_SECTION SLock;
#else
	typedef pthread_mutex_t SLock;
#endif

	inline void InitLock( SLock& Lock )
	{
#ifdef _WIN32
		InitializeCriticalSection( &Lock );
#else
		pthread_mutex_init( &Lock, NULL );
#endif
	}

	inline int32_t DestroyLock( SLock& Lock )
	{
#ifdef _WIN32
		DeleteCriticalSection( &Lock ); return 0;
#else
		return pthread_mutex_destroy( &Lock );
#endif
	}

	inline void EnterLock( SLock& Lock )
	{
#ifdef _WIN32
		EnterCriticalSection( &Lock );
#else
		pthread_mutex_lock( &Lock );
#endif
	}

	inline void LeaveLock( SLock& Lock )
	{
#ifdef _WIN32
		LeaveCriticalSection( &Lock );
#else
		pthread_mutex_unlock( &Lock );
#endif
	}
}

