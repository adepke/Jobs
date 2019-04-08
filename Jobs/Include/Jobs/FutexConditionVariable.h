// Copyright (c) 2019 Andrew Depke

#pragma once

#include <chrono>  // std::chrono::duration

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
	class FutexConditionVariable
	{
	private:
#if PLATFORM_WINDOWS
		CRITICAL_SECTION UserSpaceLock;
		CONDITION_VARIABLE ConditionVariable;
#else
		pthread_mutex_t UserSpaceLock = PTHREAD_MUTEX_INITIALIZER;
		pthread_cond_t ConditionVariable = PTHREAD_COND_INITIALIZER;
#endif

	public:
		inline FutexConditionVariable();
		FutexConditionVariable(const FutexConditionVariable&) = delete;
		FutexConditionVariable(FutexConditionVariable&&) noexcept = delete;
		inline ~FutexConditionVariable();

		FutexConditionVariable& operator=(const FutexConditionVariable&) = delete;
		FutexConditionVariable& operator=(FutexConditionVariable&&) noexcept = delete;

		inline void Lock();
		inline void Unlock();

		inline void Wait();
		template <typename Rep, typename Period>
		bool WaitFor(const std::chrono::duration<Rep, Period>& Timeout);
		inline void NotifyOne();
		inline void NotifyAll();
	};

	FutexConditionVariable::FutexConditionVariable()
	{
#if PLATFORM_WINDOWS
		InitializeCriticalSection(&UserSpaceLock);
		SetCriticalSectionSpinCount(&UserSpaceLock, 2000);

		InitializeConditionVariable(&ConditionVariable);
#else
#endif
	}

	FutexConditionVariable::~FutexConditionVariable()
	{
#if PLATFORM_WINDOWS
		DeleteCriticalSection(&UserSpaceLock);
#else
		pthread_mutex_destroy(&UserSpaceLock);
		pthread_cond_destroy(&ConditionVariable);
#endif
	}

	void FutexConditionVariable::Lock()
	{
#if PLATFORM_WINDOWS
		EnterCriticalSection(&UserSpaceLock);
#else
#endif
	}

	void FutexConditionVariable::Unlock()
	{
#if PLATFORM_WINDOWS
		LeaveCriticalSection(&UserSpaceLock);
#else
#endif
	}

	void FutexConditionVariable::Wait()
	{
#if PLATFORM_WINDOWS
		SleepConditionVariableCS(&ConditionVariable, &UserSpaceLock, INFINITE);
#else
#endif
	}

	template <typename Rep, typename Period>
	bool FutexConditionVariable::WaitFor(const std::chrono::duration<Rep, Period>& Timeout)
	{
#if PLATFORM_WINDOWS
#else
#endif

		return false;
	}

	void FutexConditionVariable::NotifyOne()
	{
#if PLATFORM_WINDOWS
		WakeConditionVariable(&ConditionVariable);
#else
#endif
	}

	void FutexConditionVariable::NotifyAll()
	{
#if PLATFORM_WINDOWS
		WakeAllConditionVariable(&ConditionVariable);
#else
#endif
	}
}