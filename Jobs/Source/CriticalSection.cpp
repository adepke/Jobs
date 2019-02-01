#include "../Include/CriticalSection.h"

CriticalSection::CriticalSection()
{
#if PLATFORM_WINDOWS
	InitializeCriticalSection(&Handle);
	SetCriticalSectionSpinCount(&Handle, 2000);
#else
	pthread_mutexattr_t HandleAttributes;
	pthread_mutexattr_init(&HandleAttributes);
	pthread_mutexattr_settype(&HandleAttributes, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&Handle, &HandleAttributes);
	pthread_mutexattr_destroy(&HandleAttributes);
#endif
}

CriticalSection::~CriticalSection()
{
#if PLATFORM_WINDOWS
	DeleteCriticalSection(&Handle);
#else
	pthread_mutex_destroy(&Handle);
#endif
}

void CriticalSection::Lock()
{
#if PLATFORM_WINDOWS
	EnterCriticalSection(&Handle);
#else
	pthread_mutex_lock(&Handle);
#endif
}

bool CriticalSection::TryLock()
{
#if PLATFORM_WINDOWS
	return TryEnterCriticalSection(&Handle);
#else
	return (pthread_mutex_trylock(&Handle) == 0);
#endif
}

void CriticalSection::Unlock()
{
#if PLATFORM_WINDOWS
	LeaveCriticalSection(&Handle);
#else
	pthread_mutex_unlock(&Handle);
#endif
}