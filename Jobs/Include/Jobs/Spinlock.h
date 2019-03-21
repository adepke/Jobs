#pragma once

#include <atomic>

namespace Jobs
{
	class Spinlock
	{
	private:
		std::atomic_flag Status = ATOMIC_FLAG_INIT;

	public:
		Spinlock() = default;
		Spinlock(const Spinlock&) = delete;
		Spinlock(Spinlock&&) noexcept = delete;

		Spinlock& operator=(const Spinlock&) = delete;
		Spinlock& operator=(Spinlock&&) noexcept = delete;

		void Lock();
		bool TryLock();
		void Unlock();
	};
}