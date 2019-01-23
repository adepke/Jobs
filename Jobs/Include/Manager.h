#pragma once

#include "../ThirdParty/ConcurrentQueue/concurrentqueue.h"
#include "Job.h"
#include "Fiber.h"
#include <vector>  // std::vector
#include <thread>  // std::thread
#include <optional>  // std::optional

struct FiberData;

class Manager
{
	friend void ManagerThreadEntry(std::size_t ID, Manager* const Owner);

private:
	FiberData* Data = nullptr;

	std::vector<moodycamel::ConcurrentQueue<Job>> JobQueues;

	std::vector<std::thread> Workers;
	std::vector<Fiber> Fibers;

	static constexpr std::size_t FiberCount = 8;
	static constexpr std::size_t FiberStackSize = 2048;

	void SetupThread(std::thread& Thread, std::size_t ThreadNumber);

public:
	Manager();
	~Manager();

	void Initialize(std::size_t ThreadCount = 0);

	template <typename U>
	void Enqueue(U&& Job);

private:
	template <typename U>
	void Enqueue(std::size_t ID, U&& Job);

	std::optional<Job> Dequeue(std::size_t ID);
};