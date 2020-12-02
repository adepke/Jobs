// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Manager.h>

#include <Jobs/FiberMutex.h>
#include <Jobs/Assert.h>
#include <Jobs/Logging.h>

#include <chrono>  // std::chrono

namespace Jobs
{
	void ManagerWorkerEntry(FiberTransfer Transfer)
	{
		void* ParentFiberCache = Transfer.context;  // Push the parent fiber on the stack, otherwise it will be overwritten when scheduling the next fiber.
		void* Data = Transfer.data;
		auto* Owner = reinterpret_cast<Manager*>(Data);

		JOBS_ASSERT(Owner, "Manager thread entry missing owner.");

		// Spin until the manager is ready.
		while (!Owner->Ready.load(std::memory_order_acquire))[[unlikely]]
		{
			std::this_thread::yield();
		}

		auto& Representation{ Owner->Workers[Owner->GetThisThreadID()] };

		// We don't have a fiber at this point, so grab an available fiber.
		auto NextFiberIndex{ Owner->GetAvailableFiber() };
		JOBS_ASSERT(Owner->IsValidID(NextFiberIndex), "Failed to retrieve an available fiber from worker.");

		Representation.FiberIndex = NextFiberIndex;

		// Schedule the fiber, which executes the work. We will resume here when the manager is shutting down.
		Owner->Fibers[NextFiberIndex].first.Schedule();

		JOBS_LOG(LogLevel::Log, "Worker Shutdown | ID: %i", Representation.GetID());

		// Return to the host thread for shutdown.
		jump_fcontext(ParentFiberCache, nullptr);
	}

	void ManagerFiberEntry(FiberTransfer Transfer)
	{
		void* WorkerFiberCache = Transfer.context;  // We need to push the original worker fiber on the stack, otherwise it gets lost when rescheduling.
		void* Data = Transfer.data;

		JOBS_ASSERT(Data, "Manager fiber entry missing data.");

		auto* Owner = reinterpret_cast<Manager*>(Data);

		while (Owner->CanContinue())
		{
			const auto ThisThreadID = Owner->GetThisThreadID();

			// Cleanup any unfinished state from the previous fiber if we need to.
			auto& ThisFiber{ Owner->Fibers[Owner->Workers[ThisThreadID].FiberIndex] };
			auto PreviousFiberIndex{ ThisFiber.first.PreviousFiberIndex };
			if (Owner->IsValidID(PreviousFiberIndex))
			{
				ThisFiber.first.PreviousFiberIndex = Manager::InvalidID;  // Reset.
				auto& PreviousFiber{ Owner->Fibers[PreviousFiberIndex] };

				// We're back, first fix up the invalidated fiber pointer.
				PreviousFiber.first.Context = Transfer.context;

				// Next make sure we restore availability to the fiber that scheduled us or enqueue it in the wait pool.
				if (PreviousFiber.first.NeedsWaitEnqueue)
				{
					PreviousFiber.first.NeedsWaitEnqueue = false;  // Reset.
					Owner->WaitingFibers.enqueue(PreviousFiberIndex);
				}

				else
				{
					PreviousFiber.second.store(true);  // #TODO: Memory order.
				}
			}

			auto& ThisThread{ Owner->Workers[ThisThreadID] };
			ThisFiber.first.WaitPoolPriority = !ThisFiber.first.WaitPoolPriority;  // Alternate wait pool priority.

			bool Continue = !ThisFiber.first.WaitPoolPriority || Owner->WaitingFibers.size_approx() == 0;  // Used to favor jobs or waiters.

			if (Continue)
			{
				auto NewJob{ std::move(Owner->Dequeue(ThisThreadID)) };
				if (NewJob)
				{
					Continue = false;  // We're satisfied, don't continue.

					// Loop until we satisfy all of our dependencies.
					bool RequiresEvaluation = true;
					while (RequiresEvaluation)
					{
						RequiresEvaluation = false;

						for (const auto& Dependency : NewJob->Dependencies)
						{
							if (auto StrongDependency{ Dependency.first.lock() }; !StrongDependency->UnsafeWait(Dependency.second, std::chrono::milliseconds{ 1 }))
							{
								// This dependency timed out, move ourselves to the wait pool.
								JOBS_LOG(LogLevel::Log, "Job dependencies timed out, moving to the wait pool.");

								auto NextFiberIndex{ Owner->GetAvailableFiber() };
								JOBS_ASSERT(Owner->IsValidID(NextFiberIndex), "Failed to retrieve an available fiber from waiting fiber.");
								auto& NextFiber = Owner->Fibers[NextFiberIndex].first;

								auto& ThisNewThread = Owner->Workers[Owner->GetThisThreadID()];  // We might resume on any worker, so we need to update this each iteration.

								ThisFiber.first.NeedsWaitEnqueue = true;  // We are waiting on a dependency, so make sure we get added to the wait pool.
								NextFiber.PreviousFiberIndex = ThisNewThread.FiberIndex;
								ThisNewThread.FiberIndex = NextFiberIndex;  // Update the fiber index.
								NextFiber.Schedule();

								JOBS_LOG(LogLevel::Log, "Job resumed from wait pool, re-evaluating dependencies.");

								// We just returned from a fiber, so we need to make sure to fix up its state. We can't wait until the main loop begins again
								// because if any of the dependencies still hold, we lose that information about the previous fiber, causing a leak.

								PreviousFiberIndex = ThisFiber.first.PreviousFiberIndex;
								if (Owner->IsValidID(PreviousFiberIndex))
								{
									ThisFiber.first.PreviousFiberIndex = Manager::InvalidID;  // Reset. Skipping this will cause a double-cleanup on the next loop beginning if the dependency doesn't hold.
									auto& PreviousFiber{ Owner->Fibers[PreviousFiberIndex] };

									// We're back, first fix up the invalidated fiber pointer.
									PreviousFiber.first.Context = Transfer.context;

									if (PreviousFiber.first.NeedsWaitEnqueue)
									{
										PreviousFiber.first.NeedsWaitEnqueue = false;  // Reset.
										Owner->WaitingFibers.enqueue(PreviousFiberIndex);
									}

									else
									{
										PreviousFiber.second.store(true);  // #TODO: Memory order.
									}
								}

								// Next we can re-evaluate the dependencies.
								RequiresEvaluation = true;
							}
						}
					}

					if (NewJob->Stream) [[unlikely]]
					{
						(*NewJob)(Owner);
					}

					else
					{
						(static_cast<Job>(*NewJob))(Owner);  // Slice.
					}

					// Finished, notify the counter if we have one. Handles expired counters (cleanup jobs) fine.
					if (auto StrongCounter{ NewJob->AtomicCounter.lock() })
					{
						StrongCounter->operator--();
					}
				}
			}

			if (Continue)
			{
				size_t WaitingFiberIndex = Manager::InvalidID;
				if (Owner->WaitingFibers.try_dequeue(WaitingFiberIndex))
				{
					auto& WaitingFiber{ Owner->Fibers[WaitingFiberIndex].first };

					JOBS_ASSERT(!ThisFiber.first.NeedsWaitEnqueue, "Logic error, should never request an enqueue if we pulled down a fiber through a dequeue.");

					// Before we schedule the waiting fiber, we need to check if it's waiting on a mutex.
					if (WaitingFiber.Mutex && !WaitingFiber.Mutex->try_lock())
					{
						JOBS_LOG(LogLevel::Log, "Waiting fiber failed to acquire mutex.");

						// Move the waiting fiber to the back of the wait queue. We don't need to mark either fiber as needing a wait enqueue
						// since we never left this fiber.
						Owner->WaitingFibers.enqueue(WaitingFiberIndex);

						// Fall through and continue on this fiber.
					}

					else
					{
						Continue = false;  // Satisfied, don't continue.

						WaitingFiber.PreviousFiberIndex = ThisThread.FiberIndex;
						ThisThread.FiberIndex = WaitingFiberIndex;
						WaitingFiber.Schedule();  // Schedule the waiting fiber. We're not a waiter, so we'll be marked as available.
					}
				}
			}

			if (Continue)
			{
				JOBS_LOG(LogLevel::Log, "Fiber sleeping.");

				Owner->QueueCV.Lock();

				// Test the shutdown condition once more under lock, as it could've been set during the transitional period.
				if (!Owner->CanContinue())
				{
					Owner->QueueCV.Unlock();

					break;
				}

				Owner->QueueCV.Wait();  // We will be woken up either by a shutdown event or if new work is available.
				Owner->QueueCV.Unlock();
			}
		}

		// End of fiber lifetime, we are switching out to the worker thread to perform any final cleanup. We cannot be scheduled again beyond this point.
		jump_fcontext(WorkerFiberCache, nullptr);

		JOBS_ASSERT(false, "Dead fiber was rescheduled.");
	}

	Manager::~Manager()
	{
		QueueCV.Lock();  // This is used to make sure we don't let any fibers slip by and not catch the shutdown notify, which causes a deadlock.
		Shutdown.store(true, std::memory_order_seq_cst);

		QueueCV.NotifyAll();  // Wake all sleepers, it's time to shutdown.
		QueueCV.Unlock();

		// Wait for all of the workers to die before deleting the fiber data.
		for (auto& Worker : Workers)
		{
			Worker.GetHandle().join();
		}
	}

	void Manager::Initialize(size_t ThreadCount)
	{
		JOBS_ASSERT(ThreadCount <= std::thread::hardware_concurrency(), "Job manager thread count should not exceed hardware concurrency.");

		for (auto Iter = 0; Iter < FiberCount; ++Iter)
		{
			Fibers[Iter].first = std::move(Fiber{ FiberStackSize, &ManagerFiberEntry, this });
			Fibers[Iter].second.store(true);  // #TODO: Memory order.
		}

		if (ThreadCount == 0)
		{
			ThreadCount = std::thread::hardware_concurrency();
		}

		Workers.reserve(ThreadCount);

		for (size_t Iter = 0; Iter < ThreadCount; ++Iter)
		{
			Workers.emplace_back(this, Iter, &ManagerWorkerEntry);
		}

		Shutdown.store(false, std::memory_order_relaxed);  // This must be set before we are ready.
		Ready.store(true, std::memory_order_release);  // This must be set last.
	}

	std::optional<JobBuilder> Manager::Dequeue(size_t ThreadID)
	{
		JobBuilder Result{};

		if (Workers[ThreadID].GetJobQueue().try_dequeue(Result))
		{
			return Result;
		}

		else
		{
			// Our queue is empty, time to steal.
			// #TODO: Implement a smart stealing algorithm.

			for (auto Iter = 1; Iter < Workers.size(); ++Iter)
			{
				if (Workers[(Iter + ThreadID) % Workers.size()].GetJobQueue().try_dequeue(Result))
				{
					return Result;
				}
			}
		}

		return std::nullopt;
	}

	size_t Manager::GetThisThreadID() const
	{
		auto ThisID{ std::this_thread::get_id() };

		for (auto& Worker : Workers)
		{
			if (Worker.GetNativeID() == ThisID)
			{
				return Worker.GetID();
			}
		}

		return InvalidID;
	}

	size_t Manager::GetAvailableFiber()
	{
		for (auto Index = 0; Index < Fibers.size(); ++Index)
		{
			auto Expected{ true };
			if (Fibers[Index].second.compare_exchange_weak(Expected, false, std::memory_order_acquire))
			{
				return Index;
			}
		}

		JOBS_LOG(LogLevel::Error, "No free fibers!");

		return InvalidID;
	}
}