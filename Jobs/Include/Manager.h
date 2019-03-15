#pragma once

#include "../ThirdParty/ConcurrentQueue/concurrentqueue.h"
#include "Job.h"
#include "Fiber.h"
#include "Worker.h"
#include <vector>  // std::vector
#include <array>  // std::array
#include <utility>  // std::move, std::pair
#include <thread>  // std::thread
#include <variant>  // std::variant
#include "CriticalSection.h"
#include <condition_variable>  // std::condition_variable
#include <mutex>  // std::mutex
#include <string>  // std::string
#include <memory>  // std::shared_ptr, std::unique_ptr
#include "Counter.h"
#include <map>  // std::map
#include <type_traits>  // std::is_same, std::decay

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
	static constexpr std::size_t FiberCount = 64;
	static constexpr std::size_t FiberStackSize = 1024 * 1024;  // 1 MB

private:
	// Shared fiber storage.
	std::unique_ptr<FiberData> Data;

	std::vector<Worker> Workers;
	std::array<std::pair<Fiber, std::atomic_bool>, FiberCount> Fibers;  // Pool of fibers paired to an availability flag.
	moodycamel::ConcurrentQueue<size_t> WaitingFibers;  // Queue of fiber indices that are waiting for some dependency or scheduled a waiting fiber.

	static constexpr auto InvalidID = std::numeric_limits<size_t>::max();

	std::atomic_bool Ready;
	std::atomic_bool Shutdown;

	// Used to cycle the worker thread to enqueue in.
	std::atomic_uint EnqueueIndex;

	std::condition_variable QueueCV;
	std::mutex QueueCVLock;

	std::map<std::string, std::shared_ptr<Counter<>>> GroupMap;

public:
	Manager() = default;
	Manager(const Manager&) = delete;
	Manager(Manager&&) noexcept = delete;  // #TODO: Implement.
	~Manager();

	Manager& operator=(const Manager&) = delete;
	Manager& operator=(Manager&&) = delete;  // #TODO: Implement.

	void Initialize(std::size_t ThreadCount = 0);

	template <typename U>
	void Enqueue(U&& Job);

	template <typename U>
	std::shared_ptr<Counter<>> Enqueue(U&& Job, const std::string& Group);

private:
	std::variant<std::monostate, Job, size_t> Dequeue();  // Returns a job, a waiting fiber index, or nothing.

	std::size_t GetThisThreadID() const;
	bool IsValidID(std::size_t ID) const;

	bool CanContinue() const;

	// Returns a fiber that is not currently scheduled.
	std::size_t GetAvailableFiber();
};

template <typename U>
void Manager::Enqueue(U&& InJob)
{
	static_assert(std::is_same_v<std::decay_t<U>, Job>, "Enqueue only supports objects of type Job");

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

	QueueCV.notify_one();  // Notify one sleeper. They will work steal if they don't get the job enqueued directly.
}

template <typename U>
std::shared_ptr<Counter<>> Manager::Enqueue(U&& InJob, const std::string& Group)
{
	static_assert(std::is_same_v<std::decay_t<U>, Job>, "Enqueue only supports objects of type Job");

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