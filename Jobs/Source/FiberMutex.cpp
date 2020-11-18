// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/FiberMutex.h>

#include <Jobs/Manager.h>
#include <Jobs/Logging.h>

void Jobs::FiberMutex::lock()
{
	if (Flag.test_and_set(std::memory_order_acquire))
	{
		// Failed to acquire the lock, add a dependency to the job and enqueue this fiber as a waiter.
		// Note that we cannot use the existing dependency system since that's used to test if jobs can
		// execute before actually kicking them off, a mutex is used for halting the execution of a job
		// that has already started.

		auto& ThisWorker = Owner->Workers[Owner->GetThisThreadID()];
		auto& ThisFiber = Owner->Fibers[ThisWorker.FiberIndex];

		ThisFiber.first.Mutex = this;  // Set the mutex, evaluated in the fiber.

		const auto NextFiberIndex = Owner->GetAvailableFiber();
		JOBS_ASSERT(Owner->IsValidID(NextFiberIndex), "Failed to retrieve an available fiber from a mutex lock.");
		auto& NextFiber = Owner->Fibers[NextFiberIndex].first;

		ThisFiber.first.NeedsWaitEnqueue = true;  // We're now waiting on a mutex, so make sure we end up in the wait queue.
		NextFiber.PreviousFiberIndex = ThisWorker.FiberIndex;
		ThisWorker.FiberIndex = NextFiberIndex;  // Update the fiber index.
		NextFiber.Schedule(ThisFiber.first);

		// We'll return here when we have acquired the mutex (locked directly via external fiber).
	}

	return;  // Acquired the lock, we're good to move on.
}

bool Jobs::FiberMutex::try_lock()
{
	return !Flag.test_and_set(std::memory_order_acquire);
}

void Jobs::FiberMutex::unlock()
{
#if __cpp_lib_atomic_flag_test
	JOBS_ASSERT(Flag.test(), "Mutex was unlocked without first being locked.");
#endif

	// We just need to clear, no need to wake any sleepers since all waiting fibers are in the wait queue.

	Flag.clear(std::memory_order_release);
}