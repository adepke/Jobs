#include "../Include/Worker.h"

#include "../Include/Manager.h"
#include "../Include/Logging.h"
#include "../Include/Assert.h"
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

Worker::Worker(Manager* const InOwner, std::size_t InID, EntryType Entry) : Owner(InOwner), ID(InID)
{
	JOBS_LOG(LogLevel::Log, "Building thread.");

	JOBS_ASSERT(InOwner, "Worker constructor needs a valid owner.");

	ThreadHandle = std::thread{ [this, Entry](auto Arg)
	{
		ThreadFiber = Fiber::FromThisThread(nullptr);

		Entry(Arg);  // We will never return here.
	}, InOwner };

#if PLATFORM_WINDOWS
	SetThreadAffinityMask(ThreadHandle.native_handle(), static_cast<std::size_t>(1) << InID);

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

Worker::Worker(Worker&& Other) noexcept
{
	Swap(Other);
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

	// TEMP: We don't need this, as the worker kills itself.
	// Delete the fiber after joining, as this will kill the thread.
	//delete ThreadFiber;
}

Worker& Worker::operator=(Worker&& Other) noexcept
{
	Swap(Other);

	return *this;
}

std::thread& Worker::GetHandle()
{
	return ThreadHandle;
}

std::thread::id Worker::GetNativeID() const
{
	return ThreadHandle.get_id();
}

std::size_t Worker::GetID() const
{
	return ID;
}

Fiber& Worker::GetThreadFiber() const
{
	return *ThreadFiber;
}

moodycamel::ConcurrentQueue<Job>& Worker::GetJobQueue()
{
	return JobQueue;
}

bool Worker::IsValidFiberIndex(std::size_t Index) const
{
	return Index != InvalidFiberIndex;
}

void Worker::Swap(Worker& Other) noexcept
{
	std::swap(Owner, Other.Owner);
	std::swap(ThreadHandle, Other.ThreadHandle);
	std::swap(ID, Other.ID);
	std::swap(ThreadFiber, Other.ThreadFiber);
	std::swap(JobQueue, Other.JobQueue);
}