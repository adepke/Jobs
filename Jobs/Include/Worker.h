#pragma once

#include <cstddef>  // std::size_t
#include <thread>  // std::thread
#include "Job.h"
#include "../ThirdParty/ConcurrentQueue/concurrentqueue.h"

class Manager;
class Fiber;

class Worker
{
	using EntryType = void(*)(Manager* const);

private:
	Manager& Owner;
	std::thread ThreadHandle;
	std::size_t ID;  // Manager-specific ID.

	std::shared_ptr<Fiber> ThreadFiber;
	moodycamel::ConcurrentQueue<Job> JobQueue;

	static constexpr std::size_t InvalidFiberIndex = std::numeric_limits<std::size_t>::max();

public:
	Worker(Manager* const, std::size_t InID, EntryType Entry);
	Worker(const Worker&) = delete;
	Worker(Worker&&) noexcept = default;
	~Worker();

	Worker& operator=(const Worker&) = delete;
	Worker& operator=(Worker&&) noexcept = delete;

	std::size_t FiberIndex = InvalidFiberIndex;  // Index into the owner's fiber pool that we're executing. We need this for rescheduling the thread fiber.

	std::thread& GetHandle();
	std::thread::id GetNativeID() const;
	std::size_t GetID() const;

	Fiber& GetThreadFiber() const;
	moodycamel::ConcurrentQueue<Job>& GetJobQueue();

	bool IsValidFiberIndex(std::size_t Index) const;
};