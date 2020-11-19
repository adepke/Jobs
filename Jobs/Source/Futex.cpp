// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Futex.h>
#include <Jobs/Platform.h>
#include <Jobs/Profiling.h>

#if JOBS_PLATFORM_WINDOWS
  #include <Jobs/WindowsMinimal.h>
#endif
#if JOBS_PLATFORM_POSIX
  #include <unistd.h>
  #include <sys/syscall.h>
  #include <linux/futex.h>
  #include <sys/time.h>
  #include <limits>
#endif

namespace Jobs
{
	bool Futex::Wait(void* CompareAddress, size_t Size, uint64_t TimeoutNs) const
	{
		JOBS_SCOPED_STAT("Futex Wait");

#if JOBS_PLATFORM_WINDOWS
		return WaitOnAddress(Address, CompareAddress, Size, static_cast<DWORD>(TimeoutNs > 0 ? (TimeoutNs >= 1e6 ? TimeoutNs : 1e6) / 1e6 : INFINITE));
#else
		timespec Timeout;
		Timeout.tv_sec = static_cast<time_t>(TimeoutNs / (uint64_t)1e9);  // Whole seconds.
		Timeout.tv_nsec = static_cast<long>(TimeoutNs % (uint64_t)1e9);  // Remaining nano seconds.

		syscall(SYS_futex, CompareAddress, FUTEX_WAIT, &Address, TimeoutNs > 0 ? &Timeout : nullptr, nullptr, 0);
#endif
	}

	void Futex::NotifyOne() const
	{
		JOBS_SCOPED_STAT("Futex Notify One");

		if (Address)
		{
#if JOBS_PLATFORM_WINDOWS
			WakeByAddressSingle(Address);
#else
			syscall(SYS_futex, Address, FUTEX_WAKE, 1);
#endif
		}
	}

	void Futex::NotifyAll() const
	{
		JOBS_SCOPED_STAT("Futex Notify All");

		if (Address)
		{
#if JOBS_PLATFORM_WINDOWS
			WakeByAddressAll(Address);
#else
			syscall(SYS_futex, Address, FUTEX_WAKE, std::numeric_limits<int>::max());
#endif
		}
	}
}