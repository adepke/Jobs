// Copyright (c) 2019-2020 Andrew Depke

#pragma once

#include <Jobs/Platform.h>

namespace Jobs
{
#if JOBS_PLATFORM_WINDOWS
	constexpr auto SizeOfUserSpaceLock = 40;
	constexpr auto SizeOfConditionVariable = 8;
#endif
#if JOBS_PLATFORM_POSIX
	constexpr auto SizeOfUserSpaceLock = 40;
	constexpr auto SizeOfConditionVariable = 48;
#endif
}

namespace Jobs
{
	class FutexConditionVariable
	{
	private:
		// Opaque storage.
		unsigned char UserSpaceLock[SizeOfUserSpaceLock];
		unsigned char ConditionVariable[SizeOfConditionVariable];

	public:
		FutexConditionVariable();
		FutexConditionVariable(const FutexConditionVariable&) = delete;
		FutexConditionVariable(FutexConditionVariable&&) noexcept = delete;
		~FutexConditionVariable();

		FutexConditionVariable& operator=(const FutexConditionVariable&) = delete;
		FutexConditionVariable& operator=(FutexConditionVariable&&) noexcept = delete;

		void Lock();
		void Unlock();

		void Wait();
		void NotifyOne();
		void NotifyAll();
	};
}