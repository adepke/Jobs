// Copyright (c) 2019 Andrew Depke

#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#include <Jobs/WindowsMinimal.h>
#else
#define PLATFORM_POSIX 1
#include <pthread.h>
#endif

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_POSIX
#define PLATFORM_POSIX 0
#endif

namespace Jobs
{
	class CriticalSection
	{
	private:
#if PLATFORM_WINDOWS
		CRITICAL_SECTION Handle;
#else
		pthread_mutex_t Handle;
#endif

	public:
		CriticalSection();
		CriticalSection(const CriticalSection&) = delete;
		CriticalSection(CriticalSection&&) noexcept = delete;
		~CriticalSection();

		CriticalSection& operator=(const CriticalSection&) = delete;
		CriticalSection& operator=(CriticalSection&&) noexcept = delete;

		void Lock();
		bool TryLock();
		void Unlock();
	};
}