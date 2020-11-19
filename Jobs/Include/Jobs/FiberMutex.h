// Copyright (c) 2019-2020 Andrew Depke

#pragma once

#include <atomic>

namespace Jobs
{
	class Manager;

	// Fiber-safe mutex, prevents deadlocking of the underlying worker.
	// Satisfies named requirements of Lockable.
	class FiberMutex
	{
	private:
		Manager* Owner = nullptr;
		std::atomic_flag Flag = ATOMIC_FLAG_INIT;

	public:
		FiberMutex(Manager* InOwner) : Owner(InOwner) {}
		~FiberMutex() = default;

		FiberMutex(const FiberMutex&) = delete;
		FiberMutex(FiberMutex&&) noexcept = delete;

		FiberMutex& operator=(const FiberMutex&) = delete;
		FiberMutex& operator=(FiberMutex&&) noexcept = delete;

		void lock();
		bool try_lock();
		void unlock();
	};
}