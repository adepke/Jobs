// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Worker.h>

#include <Jobs/Manager.h>
#include <Jobs/Logging.h>
#include <Jobs/Assert.h>
#include <Jobs/Fiber.h>

#if JOBS_PLATFORM_WINDOWS
  #include <Jobs/WindowsMinimal.h>
#endif
#if JOBS_PLATFORM_POSIX
  #include <pthread.h>
#endif

namespace Jobs
{
	Worker::Worker(Manager* InOwner, size_t InID, EntryType Entry) : Owner(InOwner), ID(InID)
	{
		JOBS_SCOPED_STAT("Worker Creation");

		JOBS_LOG(LogLevel::Log, "Building thread.");

		JOBS_ASSERT(InOwner, "Worker constructor needs a valid owner.");

		ThreadFiber = new Fiber{ Manager::FiberStackSize, Entry, Owner };

		ThreadHandle = std::thread{ [this]()
		{
			ThreadFiber->Schedule();  // Schedule our fiber from a new thread. We will resume here once the worker is shutdown.

			JOBS_LOG(LogLevel::Log, "Worker fiber returned, shutting down kernel fiber");
		} };

#if JOBS_PLATFORM_WINDOWS
		SetThreadAffinityMask(ThreadHandle.native_handle(), static_cast<size_t>(1) << InID);
		SetThreadDescription(ThreadHandle.native_handle(), L"Jobs Worker");
#else
		cpu_set_t CPUSet;
		CPU_ZERO(&CPUSet);
		CPU_SET(InID, &CPUSet);

		JOBS_ASSERT(pthread_setaffinity_np(ThreadHandle.native_handle(), sizeof(CPUSet), &CPUSet) == 0, "Error occurred in pthread_setaffinity_np().");

		pthread_setname_np(ThreadHandle.native_handle(), "Jobs Worker");
#endif
	}

	Worker::~Worker()
	{
		if (ThreadHandle.native_handle())
		{
			// Only log if we're not a moved worker shell.
			JOBS_LOG(LogLevel::Log, "Destroying thread.");
		}

		// The thread may have already finished, so validate our handle first.
		if (ThreadHandle.joinable())
		{
			ThreadHandle.join();
		}

		delete ThreadFiber;
	}

	void Worker::Swap(Worker& Other) noexcept
	{
		std::swap(Owner, Other.Owner);
		std::swap(ThreadHandle, Other.ThreadHandle);
		std::swap(ID, Other.ID);
		std::swap(ThreadFiber, Other.ThreadFiber);
		std::swap(JobQueue, Other.JobQueue);
	}
}