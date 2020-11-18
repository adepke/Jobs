// Copyright (c) 2019-2020 Andrew Depke

#pragma once

#include <atomic>  // std::atomic_flag
#include <cstddef>  // std::size_t
#include <memory>  // std::shared_ptr
#include <limits>  // std::numeric_limits

namespace Jobs
{
	class Manager;
	class FiberMutex;

	class Fiber
	{
		using EntryType = void(*)(void*);

	private:
		void* Context = nullptr;
		Manager* Owner = nullptr;

	public:
		bool WaitPoolPriority = false;  // Used for alternating wait pool. Does not need to be atomic.
		size_t PreviousFiberIndex = std::numeric_limits<size_t>::max();  // Used to track the fiber that scheduled us.
		bool NeedsWaitEnqueue = false;  // Used to mark if we need to have availability restored or added to the wait pool.

		FiberMutex* Mutex = nullptr;  // Used to determine if we're waiting on a mutex.

	public:
		Fiber() = default;  // Used for converting a thread to a fiber.
		Fiber(size_t StackSize, EntryType Entry, Manager* InOwner);
		Fiber(const Fiber&) = delete;
		Fiber(Fiber&& Other) noexcept;
		~Fiber();

		Fiber& operator=(const Fiber&) = delete;
		Fiber& operator=(Fiber&& Other) noexcept;

		void Schedule(const Fiber& From);

		void Swap(Fiber& Other) noexcept;

		static Fiber* FromThisThread(void* Arg);
	};
}