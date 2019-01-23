#include "../Include/Manager.h"

#include "../include/Assert.h"
#include "../Include/Fiber.h"

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#include "../Include/WindowsMinimal.h"
#else
#define PLATFORM_POSIX 1
#include <pthread.h>
#endif

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_POSIX
#define PLATFORM_POSIX 0
#endif

struct FiberData
{
	Manager* Owner;
};

void ManagerFiberEntry(void* Data)
{
	JOBS_ASSERT(Data, "Manager fiber entry missing data.");
}

void ManagerThreadEntry(std::size_t ID, Manager* const Owner)
{
	JOBS_ASSERT(Owner, "Manager thread entry missing owner.");
}

void Manager::SetupThread(std::thread& Thread, std::size_t ThreadNumber)
{
#if PLATFORM_WINDOWS
	SetThreadAffinityMask(Thread.native_handle(), static_cast<std::size_t>(1) << ThreadNumber);

#else
	cpu_set_t CPUSet;
	CPU_ZERO(&CPUSet);
	CPU_SET(ThreadNumber, &CPUSet);

	JOBS_ASSERT(pthread_setaffinity_np(Thread.native_handle(), sizeof(CPUSet), &CPUSet) == 0, "Error occurred in pthread_setaffinity_np().");
#endif
}

Manager::Manager() {}

Manager::~Manager()
{
	for (auto& Worker : Workers)
	{
		Worker.join();
	}

	delete Data;
}

void Manager::Initialize(std::size_t ThreadCount)
{
	JOBS_ASSERT(ThreadCount <= std::thread::hardware_concurrency(), "Job manager thread count should not exceed hardware concurrency.");

	Data = new FiberData{ this };

	for (auto Iter = 0; Iter < FiberCount; ++Iter)
	{
		Fibers.push_back(Fiber{ FiberStackSize, &ManagerFiberEntry, Data });
	}

	if (ThreadCount == 0)
	{
		ThreadCount = std::thread::hardware_concurrency();
	}

	JobQueues.reserve(ThreadCount);
	Workers.reserve(ThreadCount);
	
	for (auto Iter = 0; Iter < ThreadCount; ++Iter)
	{
		JobQueues.push_back(moodycamel::ConcurrentQueue<Job>{});
		Workers.push_back(std::thread{ &ManagerThreadEntry, Iter, this });

		SetupThread(Workers[Iter], Iter);
	}
}

template <typename U>
void Manager::Enqueue(U&& Job)
{
	// #TODO: Cycle the default thread queue to enqueue in.
	JobQueues[0].enqueue(Job);
}

template <typename U>
void Manager::Enqueue(std::size_t ID, U&& Job)
{
	JobQueues[ID].enqueue(Job);
}

std::optional<Job> Manager::Dequeue(std::size_t ID)
{
	Job Result{};

	if (JobQueues[ID].try_dequeue(Result))
	{
		return Result;
	}

	else
	{
		// Our queue is empty, time to steal.

		for (auto Iter = 0; Iter < Workers.size(); ++Iter)
		{
			if (JobQueues[(Iter + ID) % Workers.size()].try_dequeue(Result))
			{
				return Result;
			}
		}
	}

	return std::nullopt;
}