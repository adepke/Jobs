// Copyright (c) 2019 Andrew Depke

#pragma once

#include "../../ThirdParty/ConcurrentQueue/concurrentqueue.h"
#include <Jobs/Job.h>
#include <Jobs/Fiber.h>
#include <Jobs/Worker.h>
#include <vector>  // std::vector
#include <array>  // std::array
#include <utility>  // std::move, std::pair
#include <thread>  // std::thread
#include <variant>  // std::variant
#include <mutex>  // std::mutex
#include <string>  // std::string
#include <memory>  // std::shared_ptr, std::unique_ptr
#include <Jobs/Counter.h>
#include <map>  // std::map
#include <type_traits>  // std::is_same, std::decay
#include <optional>  // std::optional

namespace Jobs
{
	// Shared common data.
	struct FiberData
	{
		Manager* const Owner;

		FiberData(Manager* const InOwner) : Owner(InOwner) {};
	};

	class Manager
	{
		friend class Worker;
		friend void ManagerWorkerEntry(Manager* const Owner);
		friend void ManagerFiberEntry(void* Data);

		// #TODO: Move these into template traits.
		static constexpr size_t FiberCount = 64;
		static constexpr size_t FiberStackSize = 1024 * 1024;  // 1 MB

	private:
		// Shared fiber storage.
		std::unique_ptr<FiberData> Data;

		std::vector<Worker> Workers;
		std::array<std::pair<Fiber, std::atomic_bool>, FiberCount> Fibers;  // Pool of fibers paired to an availability flag.
		moodycamel::ConcurrentQueue<size_t> WaitingFibers;  // Queue of fiber indices that are waiting for some dependency or scheduled a waiting fiber.

		static constexpr auto InvalidID = std::numeric_limits<size_t>::max();

		std::atomic_bool Ready;
		alignas(std::hardware_destructive_interference_size) std::atomic_bool Shutdown;

		// Used to cycle the worker thread to enqueue in.
		std::atomic_uint EnqueueIndex;

		alignas(std::hardware_destructive_interference_size) FutexConditionVariable QueueCV;

		// #TODO: Use a more efficient hash map data structure.
		std::map<std::string, std::shared_ptr<Counter<>>> GroupMap;

	public:
		Manager() = default;
		Manager(const Manager&) = delete;
		Manager(Manager&&) noexcept = delete;  // #TODO: Implement.
		~Manager();

		Manager& operator=(const Manager&) = delete;
		Manager& operator=(Manager&&) = delete;  // #TODO: Implement.

		void Initialize(size_t ThreadCount = 0);

		template <typename U>
		void Enqueue(U&& InJob);

		template <typename U>
		void Enqueue(U&& InJob, const std::shared_ptr<Counter<>>& InCounter);

		template <typename U>
		std::shared_ptr<Counter<>> Enqueue(U&& InJob, const std::string& Group);

	private:
		std::optional<Job> Dequeue(size_t ThreadID);

		size_t GetThisThreadID() const;
		inline bool IsValidID(size_t ID) const;

		inline bool CanContinue() const;

		size_t GetAvailableFiber();  // Returns a fiber that is not currently scheduled.
	};

	template <typename U>
	void Manager::Enqueue(U&& InJob)
	{
		if constexpr (!std::is_same_v<std::decay_t<U>, Job>)
		{
			static_assert(false, "Enqueue only supports objects of type Job");
		}

		else
		{
			auto ThisThreadID{ GetThisThreadID() };

			if (IsValidID(ThisThreadID))
			{
				Workers[ThisThreadID].GetJobQueue().enqueue(std::forward<U>(InJob));
			}

			else
			{
				auto CachedEI{ EnqueueIndex.load(std::memory_order_acquire) };

				// Note: We might lose an increment here if this runs in parallel, but we would rather suffer that instead of locking.
				EnqueueIndex.store((CachedEI + 1) % Workers.size(), std::memory_order_release);

				Workers[CachedEI].GetJobQueue().enqueue(std::forward<U>(InJob));
			}

			// #NOTE: Safeguarding the notify can destroy performance in high enqueue situations. This leaves a blind spot potential,
			// but the risk is worth it. Even if a blind spot signal happens, the worker will just sleep until a new enqueue arrives, where it can recover.
			//QueueCV.Lock();
			QueueCV.NotifyOne();  // Notify one sleeper. They will work steal if they don't get the job enqueued directly.
			//QueueCV.Unlock();
		}
	}

	template <typename U>
	void Manager::Enqueue(U&& InJob, const std::shared_ptr<Counter<>>& InCounter)
	{
		if constexpr (!std::is_same_v<std::decay_t<U>, Job>)
		{
			static_assert(false, "Enqueue only supports objects of type Job");
		}

		else
		{
			InCounter->operator++();
			InJob.AtomicCounter = InCounter;

			Enqueue(InJob);
		}
	}

	template <typename U>
	std::shared_ptr<Counter<>> Manager::Enqueue(U&& InJob, const std::string& Group)
	{
		if constexpr (!std::is_same_v<std::decay_t<U>, Job>)
		{
			static_assert(false, "Enqueue only supports objects of type Job");
		}

		else
		{
			std::shared_ptr<Counter<>> GroupCounter;
			auto AllocateCounter{ []() { return std::make_shared<Counter<>>(1); } };

			if (Group.empty())
			{
				GroupCounter = std::move(AllocateCounter());
			}

			else
			{
				auto GroupIter{ GroupMap.find(Group) };

				if (GroupIter != GroupMap.end())
				{
					GroupCounter = GroupIter->second;
					GroupCounter->operator++();
				}

				else
				{
					GroupCounter = std::move(AllocateCounter());
					GroupMap.insert({ Group, GroupCounter });
				}
			}

			InJob.AtomicCounter = GroupCounter;
			Enqueue(InJob);

			return GroupCounter;
		}
	}

	bool Manager::IsValidID(size_t ID) const
	{
		return ID != InvalidID;
	}

	bool Manager::CanContinue() const
	{
		return !Shutdown.load(std::memory_order_acquire);
	}
}