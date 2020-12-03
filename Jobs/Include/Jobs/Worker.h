// Copyright (c) 2019-2020 Andrew Depke

#pragma once

#include "../../ThirdParty/ConcurrentQueue/concurrentqueue.h"
#include <Jobs/JobBuilder.h>

#include <cstddef>  // std::size_t
#include <thread>  // std::thread
#include <atomic>  // std::atomic

namespace Jobs
{
	class Manager;
	class Fiber;

	class Worker
	{
		using EntryType = void(*)(FiberTransfer);

	private:
		Manager* Owner = nullptr;
		std::thread ThreadHandle;
		size_t ID;  // Manager-specific ID.

		Fiber* ThreadFiber = nullptr;
		moodycamel::ConcurrentQueue<JobBuilder> JobQueue;

		static constexpr size_t InvalidFiberIndex = std::numeric_limits<size_t>::max();

	public:
		Worker(Manager* const, size_t InID, EntryType Entry);
		Worker(const Worker&) = delete;
		Worker(Worker&& Other) noexcept { Swap(Other); }
		~Worker();

		Worker& operator=(const Worker&) = delete;
		Worker& operator=(Worker&& Other) noexcept
		{
			Swap(Other);

			return *this;
		}

		size_t FiberIndex = InvalidFiberIndex;  // Index into the owner's fiber pool that we're executing. Allows for fibers to become aware of their own ID.

		std::thread& GetHandle() { return ThreadHandle; }
		std::thread::id GetNativeID() const { return ThreadHandle.get_id(); }
		size_t GetID() const { return ID; }

		Fiber& GetThreadFiber() const { return *ThreadFiber; }
		moodycamel::ConcurrentQueue<JobBuilder>& GetJobQueue() { return JobQueue; }

		constexpr bool IsValidFiberIndex(size_t Index) const { return Index != InvalidFiberIndex; }

		void Swap(Worker& Other) noexcept;
	};
}