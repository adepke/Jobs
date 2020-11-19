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

		ThreadHandle = std::thread{ [this, Entry](auto* Arg)
		{
			ThreadFiber = Fiber::FromThisThread(nullptr);

			Ready.store(true, std::memory_order_seq_cst);  // We saved the thread fiber pointer, the worker can now move us.

			Entry(Arg);  // We will never return here.
		}, Owner };

#if JOBS_PLATFORM_WINDOWS
		SetThreadAffinityMask(ThreadHandle.native_handle(), static_cast<size_t>(1) << InID);

#if _DEBUG || NDEBUG
		SetThreadDescription(ThreadHandle.native_handle(), L"Jobs Worker");
#endif

#else
		cpu_set_t CPUSet;
		CPU_ZERO(&CPUSet);
		CPU_SET(InID, &CPUSet);

		JOBS_ASSERT(pthread_setaffinity_np(ThreadHandle.native_handle(), sizeof(CPUSet), &CPUSet) == 0, "Error occurred in pthread_setaffinity_np().");
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