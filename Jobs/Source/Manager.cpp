// Copyright (c) 2019 Andrew Depke

#include <Jobs/Manager.h>

#include <Jobs/Assert.h>
#include <Jobs/Logging.h>
#include <Jobs/Fiber.h>
#include <Jobs/Counter.h>

#include <chrono>  // std::chrono

namespace Jobs
{
	void ManagerWorkerEntry(Manager* const Owner)
	{
		JOBS_ASSERT(Owner, "Manager thread entry missing owner.");

		// Spin until the manager is ready.
		while (!Owner->Ready.load(std::memory_order_acquire)) [[unlikely]]
		{
			std::this_thread::yield();
		}

			// ThisFiber allows us to schedule other fibers.
		auto& Representation{ Owner->Workers[Owner->GetThisThreadID()] };

		// We don't have a fiber at this point, so grab an available fiber.
		auto NextFiberIndex{ Owner->GetAvailableFiber() };
		JOBS_ASSERT(Owner->IsValidID(NextFiberIndex), "Failed to retrieve an available fiber from worker.");

		Representation.FiberIndex = NextFiberIndex;

		// Schedule the fiber, which executes the work. We will resume here when the manager is shutting down.
		Owner->Fibers[NextFiberIndex].first.Schedule(Owner->Workers[Owner->GetThisThreadID()].GetThreadFiber());

		JOBS_LOG(LogLevel::Log, "Worker Shutdown | ID: %i", Representation.GetID());

		// Kill ourselves. #TODO: This is a little dirty, we should fix this so that we perform the standard thread cleanup procedure and get a return code of 0.
		delete &Representation.GetThreadFiber();
	}

	void ManagerFiberEntry(void* Data)
	{
		JOBS_ASSERT(Data, "Manager fiber entry missing data.");

		auto FData = static_cast<FiberData*>(Data);

		while (FData->Owner->CanContinue())
		{
			// Cleanup an unfinished state from the previous fiber if we need to.
			auto& ThisFiber{ FData->Owner->Fibers[FData->Owner->Workers[FData->Owner->GetThisThreadID()].FiberIndex] };
			const auto PreviousFiberIndex{ ThisFiber.first.PreviousFiberIndex };
			if (FData->Owner->IsValidID(PreviousFiberIndex))
			{
				ThisFiber.first.PreviousFiberIndex = Manager::InvalidID;  // Reset.
				auto& PreviousFiber{ FData->Owner->Fibers[PreviousFiberIndex] };

				// We're back, first make sure we restore availability to the fiber that scheduled us or enqueue it in the wait pool.
				if (PreviousFiber.first.NeedsWaitEnqueue)
				{
					PreviousFiber.first.NeedsWaitEnqueue = false;  // Reset.
					FData->Owner->WaitingFibers.enqueue(PreviousFiberIndex);
				}

				else
				{
					PreviousFiber.second.store(true);  // #TODO: Memory order.
				}
			}

			auto DequeueResult{ std::move(FData->Owner->Dequeue()) };
			if (auto* JobResult{ std::get_if<Job>(&DequeueResult) })
			{
				// Loop until we satisfy all of our dependencies.
				bool RequiresEvaluation = true;
				while (RequiresEvaluation)
				{
					RequiresEvaluation = false;

					for (const auto& Dependency : JobResult->Dependencies)
					{
						auto StrongDependency{ Dependency.first.lock() };
						JOBS_LOG(LogLevel::Log, "Current: %d Required: %d", StrongDependency->Get(), Dependency.second);

						if (auto StrongDependency{ Dependency.first.lock() }; !StrongDependency->WaitUserSpace(Dependency.second, std::chrono::milliseconds{ 1 }))
						{
							// This dependency timed out, move ourselves to the wait pool.
							JOBS_LOG(LogLevel::Log, "Job dependencies timed out, moving to the wait pool.");

							auto NextFiberIndex{ FData->Owner->GetAvailableFiber() };
							JOBS_ASSERT(FData->Owner->IsValidID(NextFiberIndex), "Failed to retrieve an available fiber from waiting fiber.");

							auto& ThisThread{ FData->Owner->Workers[FData->Owner->GetThisThreadID()] };

							ThisFiber.first.NeedsWaitEnqueue = true;  // We are waiting on a dependency, so make sure we get added to the wait pool.
							FData->Owner->Fibers[NextFiberIndex].first.PreviousFiberIndex = ThisThread.FiberIndex;
							ThisThread.FiberIndex = NextFiberIndex;  // Update the fiber index.
							FData->Owner->Fibers[NextFiberIndex].first.Schedule(FData->Owner->Fibers[ThisThread.FiberIndex].first);

							JOBS_LOG(LogLevel::Log, "Job resumed from wait pool, re-evaluating dependencies.");

							// Next we can re-evaluate the dependencies.
							RequiresEvaluation = true;
						}
					}
				}

				(*JobResult)();

				// Finished, notify the counter if we have one.
				if (auto StrongCounter{ JobResult->AtomicCounter.lock() })
				{
					StrongCounter->operator--();
				}
			}

			else if (auto* WaitingFiberIndex{ std::get_if<size_t>(&DequeueResult) })
			{
				auto& ThisThread{ FData->Owner->Workers[FData->Owner->GetThisThreadID()] };
				auto& ThisFiber{ FData->Owner->Fibers[ThisThread.FiberIndex] };
				auto& WaitingFiber{ FData->Owner->Fibers[*WaitingFiberIndex].first };

				JOBS_ASSERT(!ThisFiber.first.NeedsWaitEnqueue, "Logic error, should never request an enqueue if we pulled down a fiber through a dequeue.");

				WaitingFiber.PreviousFiberIndex = ThisThread.FiberIndex;
				ThisThread.FiberIndex = *WaitingFiberIndex;
				WaitingFiber.Schedule(ThisFiber.first);  // Schedule the waiting fiber. It might be a long time before we resume, if ever.
			}

			// #BUG: Going from dequeue fail to sleeping is not an atomic operation, so we potentially miss work enqueued in this period.
			else
			{
				JOBS_LOG(LogLevel::Log, "Fiber sleeping.");

				std::unique_lock Lock{ FData->Owner->QueueCVLock };

				// Test the shutdown condition once more under lock, as it could've been set during the transitional period.
				if (!FData->Owner->CanContinue())
				{
					break;
				}

				FData->Owner->QueueCV.wait(Lock);  // We will be woken up either by a shutdown event or if new work is available.
			}
		}

		// End of fiber lifetime, we are switching out to the worker thread to perform any final cleanup. We cannot be scheduled again beyond this point.
		auto& ActiveWorker{ FData->Owner->Workers[FData->Owner->GetThisThreadID()] };

		ActiveWorker.GetThreadFiber().Schedule(FData->Owner->Fibers[ActiveWorker.FiberIndex].first);

		JOBS_ASSERT(false, "Dead fiber was rescheduled.");
	}

	Manager::~Manager()
	{
		std::unique_lock Lock{ QueueCVLock };  // This is used to make sure we don't let any fibers slip by and not catch the shutdown notify, which causes a deadlock.
		Shutdown.store(true, std::memory_order_seq_cst);

		QueueCV.notify_all();  // Wake all sleepers, it's time to shutdown.
		Lock.unlock();

		// Wait for all of the workers to die before deleting the fiber data.
		for (auto& Worker : Workers)
		{
			Worker.GetHandle().join();
		}
	}

	void Manager::Initialize(size_t ThreadCount)
	{
		JOBS_ASSERT(ThreadCount <= std::thread::hardware_concurrency(), "Job manager thread count should not exceed hardware concurrency.");

		Data = std::move(std::make_unique<FiberData>(this));

		for (auto Iter = 0; Iter < FiberCount; ++Iter)
		{
			Fibers[Iter].first = std::move(Fiber{ FiberStackSize, &ManagerFiberEntry, Data.get() });
			Fibers[Iter].second.store(true);  // #TODO: Memory order.
		}

		if (ThreadCount == 0)
		{
			ThreadCount = std::thread::hardware_concurrency();
		}

		Workers.reserve(ThreadCount);

		for (size_t Iter = 0; Iter < ThreadCount; ++Iter)
		{
			Worker NewWorker{ this, Iter, &ManagerWorkerEntry };

			// Fix for a rare data race when the swap occurs while the worker is saving the fiber pointer.
			while (!NewWorker.IsReady()) [[unlikely]]
			{
				// Should almost never end up spinning here.
				std::this_thread::yield();
			}

			Workers.push_back(std::move(NewWorker));
		}

		Shutdown.store(false, std::memory_order_relaxed);  // This must be set before we are ready.
		Ready.store(true, std::memory_order_release);  // This must be set last.
	}

	std::variant<std::monostate, Job, size_t> Manager::Dequeue()
	{
		Job Result{};
		auto ThisThreadID{ GetThisThreadID() };

		auto& ThisFiber{ Fibers[Workers[ThisThreadID].FiberIndex] };
		ThisFiber.first.WaitPoolPriority = !ThisFiber.first.WaitPoolPriority;  // Alternate wait pool priority.

		if (!ThisFiber.first.WaitPoolPriority || WaitingFibers.size_approx() == 0)
		{
			if (Workers[ThisThreadID].GetJobQueue().try_dequeue(Result))
			{
				return Result;
			}

			else
			{
				// Our queue is empty, time to steal.
				// #TODO: Implement a smart stealing algorithm.

				for (auto Iter = 1; Iter < Workers.size(); ++Iter)
				{
					if (Workers[(Iter + ThisThreadID) % Workers.size()].GetJobQueue().try_dequeue(Result))
					{
						return Result;
					}
				}
			}

			// Fall through if we don't have any work available, this will test the wait pool before sleeping.
		}

		if (size_t WaitingFiberIndex{ 0 }; WaitingFibers.try_dequeue(WaitingFiberIndex))
		{
			return WaitingFiberIndex;
		}

		return std::monostate{};
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

	bool Manager::IsValidID(size_t ID) const
	{
		return ID != InvalidID;
	}

	bool Manager::CanContinue() const
	{
		return !Shutdown.load(std::memory_order_acquire);
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