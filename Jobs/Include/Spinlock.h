#pragma once

#include <atomic>

class Spinlock
{
private:
	std::atomic_flag Status = ATOMIC_FLAG_INIT;

public:
	void Lock();
	bool TryLock();
	void Unlock();
};