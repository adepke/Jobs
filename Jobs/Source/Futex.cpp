#include "../Include/Futex.h"

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#include "../Include/WindowsMinimal.h"
#else
#define PLATFORM_POSIX 1
#include <linux/futex.h>
#include <sys/time.h>
#endif

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_POSIX
#define PLATFORM_POSIX 0
#endif

bool Futex::Wait(void* CompareAddress, size_t Size, uint64_t TimeoutNs) const
{
#if PLATFORM_WINDOWS
	return WaitOnAddress(Address, CompareAddress, Size, TimeoutNs > 0 ? (TimeoutNs >= 1e6 ? TimeoutNs : 1e6) / 1e6 : INFINITE);
#else
	timespec Timeout;
	Timeout.tv_sec = static_cast<int>(Timeout / 1e9);  // Whole seconds.
	Timeout.tv_nsec = static_cast<long>(TimeoutNs % 1e9);  // Remaining nano seconds.

	futex(CompareAddress, FUTEX_WAIT, , TimeoutNs > 0 ? &Timeout : nullptr, , );
	sys_futex()?
#endif
}

void Futex::NotifyOne() const
{
	if (Address)
	{
#if PLATFORM_WINDOWS
		WakeByAddressSingle(Address);
#else
#endif
	}
}

void Futex::NotifyAll() const
{
	if (Address)
	{
#if PLATFORM_WINDOWS
		WakeByAddressAll(Address);
#else
#endif
	}
}