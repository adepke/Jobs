#pragma once

#include <cstddef>  // std::size_t
#include <thread>  // std::thread
#include <atomic>  // std::atomic
#include <Jobs/Job.h>
#include "../../ThirdParty/ConcurrentQueue/concurrentqueue.h"

namespace Jobs
{
	class Manager;
	class Fiber;

	class Worker
	{
		using EntryType = void(*)(Manager* const);

	private:
		std::atomic_bool Ready;
		Manager* Owner = nullptr;
		std::thread ThreadHandle;
		size_t ID;  // Manager-specific ID.

		Fiber* ThreadFiber = nullptr;
		moodycamel::ConcurrentQueue<Job> JobQueue;

		static constexpr size_t InvalidFiberIndex = std::numeric_limits<size_t>::max();

	public:
		Worker(Manager* const, size_t InID, EntryType Entry);
		Worker(const Worker&) = delete;
		Worker(Worker&& Other) noexcept;
		~Worker();

		Worker& operator=(const Worker&) = delete;
		Worker& operator=(Worker&& Other) noexcept;

		size_t FiberIndex = InvalidFiberIndex;  // Index into the owner's fiber pool that we're executing. We need this for rescheduling the thread fiber.

		bool IsReady() const;
		std::thread& GetHandle();
		std::thread::id GetNativeID() const;
		size_t GetID() const;

		Fiber& GetThreadFiber() const;
		moodycamel::ConcurrentQueue<Job>& GetJobQueue();

		bool IsValidFiberIndex(size_t Index) const;

		void Swap(Worker& Other) noexcept;
	};
}