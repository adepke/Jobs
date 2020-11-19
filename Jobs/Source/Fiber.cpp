// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Fiber.h>

#include <Jobs/Logging.h>
#include <Jobs/Assert.h>
#include <Jobs/Profiling.h>

#include <utility>  // std::swap

#if JOBS_PLATFORM_WINDOWS
  #include <Jobs/WindowsMinimal.h>
#endif
#if JOBS_PLATFORM_POSIX
  #include <pthread.h>
  #include <ucontext.h>
#endif

namespace Jobs
{
	Fiber::Fiber(size_t StackSize, EntryType Entry, Manager* InOwner) : Owner(InOwner)
	{
		JOBS_SCOPED_STAT("Fiber Creation");

		JOBS_LOG(LogLevel::Log, "Building fiber.");
		JOBS_ASSERT(StackSize > 0, "Stack size must be greater than 0.");

#if JOBS_PLATFORM_WINDOWS
		Context = CreateFiber(StackSize, Entry, reinterpret_cast<void*>(Owner));
#else
		ucontext_t* NewContext = new ucontext_t{};
		JOBS_ASSERT(getcontext(NewContext) == 0, "Error occurred in getcontext().");

		auto* Stack = new std::byte[StackSize];
		NewContext->uc_stack.ss_sp = Stack;
		NewContext->uc_stack.ss_size = sizeof(std::byte) * StackSize;
		NewContext->uc_link = nullptr;  // Exit thread on fiber return.

		makecontext(NewContext, Entry, 0);

		Context = static_cast<void*>(NewContext);
#endif

		JOBS_ASSERT(Context, "Failed to build fiber.");
	}

	Fiber::Fiber(Fiber&& Other) noexcept
	{
		Swap(Other);
	}

	Fiber::~Fiber()
	{
#if JOBS_PLATFORM_WINDOWS
		if (Context)
		{
			// We only want to log when an actual fiber was destroyed, not the shell of a fiber that was moved.
			JOBS_LOG(LogLevel::Log, "Destroying fiber.");

			DeleteFiber(Context);
		}
#else
		delete[] static_cast<ucontext_t*>(Context)->uc_stack.ss_sp;
		delete static_cast<ucontext_t*>(Context);
#endif
	}

	Fiber& Fiber::operator=(Fiber&& Other) noexcept
	{
		Swap(Other);

		return *this;
	}

	void Fiber::Schedule(const Fiber& From)
	{
		JOBS_SCOPED_STAT("Fiber Schedule");

		JOBS_LOG(LogLevel::Log, "Scheduling fiber.");

#if JOBS_PLATFORM_WINDOWS
		// #TODO Should this be windows only? Explore behavior of posix fibers swapping to themselves.
		JOBS_ASSERT(Context != GetCurrentFiber(), "Fibers scheduling themselves causes unpredictable issues.");

		SwitchToFiber(Context);
#else
		JOBS_ASSERT(swapcontext(static_cast<ucontext_t*>(From.Context), static_cast<ucontext_t*>(Context)) == 0, "Failed to schedule fiber.");
#endif
	}

	void Fiber::Swap(Fiber& Other) noexcept
	{
		std::swap(Context, Other.Context);
		std::swap(Owner, Other.Owner);
	}

	Fiber* Fiber::FromThisThread(void* Arg)
	{
		JOBS_SCOPED_STAT("Fiber From Thread");

		auto* Result{ new Fiber{} };

#if JOBS_PLATFORM_WINDOWS
		Result->Context = ConvertThreadToFiber(Arg);
#else
		Result->Context = new ucontext_t{};
		JOBS_ASSERT(getcontext(Result->Context) == 0, "Error occurred in getcontext().");
#endif

		return Result;
	}
}