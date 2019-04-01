// Copyright (c) 2019 Andrew Depke

#include <Jobs/Spinlock.h>

#include <Jobs/Assert.h>

#include <thread>  // std::this_thread

namespace Jobs
{
	void Spinlock::Lock()
	{
		while (Status.test_and_set(std::memory_order_acquire)) [[unlikely]]
		{
			std::this_thread::yield();
		}
	}

	bool Spinlock::TryLock()
	{
		return !Status.test_and_set(std::memory_order_acquire);
	}

	void Spinlock::Unlock()
	{
		Status.clear(std::memory_order_release);
	}
}