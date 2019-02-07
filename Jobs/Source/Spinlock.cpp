#include "../include/Spinlock.h"

#include "../Include/Assert.h"

#include <thread>  // std::this_thread

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