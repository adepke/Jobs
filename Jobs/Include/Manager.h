#pragma once

#include "../ThirdParty/ConcurrentQueue/concurrentqueue.h"
#include "Job.h"
#include "Fiber.h"
#include "Worker.h"
#include <vector>  // std::vector
#include <thread>  // std::thread
#include <optional>  // std::optional
#include <utility>  // std::pair
#include "CriticalSection.h"

struct FiberData;

class Manager
{
	friend class Worker;
	friend void ManagerWorkerEntry(Manager* const Owner);
	friend void ManagerFiberEntry(void* Data);

private:
	// Shared fiber storage.
	FiberData* Data = nullptr;

	std::vector<Worker> Workers;
	std::vector<Fiber> Fibers;  // Pool of fibers, they may or may not be available.
	std::vector<Fiber> WaitingFibers;  // #TODO: Pool of fibers that are waiting for some dependency. Separated from the main pool for cache effectiveness.

	static constexpr std::size_t FiberCount = 16;
	static constexpr std::size_t FiberStackSize = 1024 * 1024;  // 1 MB

	static constexpr std::size_t InvalidID = std::numeric_limits<std::size_t>::max();

	CriticalSection FiberPoolLock;

	std::atomic_bool Ready;

public:
	Manager();
	~Manager();

	void Initialize(std::size_t ThreadCount = 0);

	template <typename U>
	void Enqueue(U&& Job);

private:
	std::optional<Job> Dequeue();

	std::size_t GetThisThreadID() const;
	bool IsValidID(std::size_t ID) const;

	bool CanContinue() const;

	std::size_t GetAvailableFiber();
};

template <typename U>
void Manager::Enqueue(U&& Job)
{
	auto ThisThreadID{ GetThisThreadID() };

	if (IsValidID(ThisThreadID))
	{
		Workers[ThisThreadID].GetJobQueue().enqueue(std::forward<U>(Job));
	}

	else
	{
		// #TODO: Cycle the default thread queue to enqueue in.
		Workers[0].GetJobQueue().enqueue(std::forward<U>(Job));
	}
}