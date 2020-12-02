// Copyright (c) 2019-2020 Andrew Depke

#include <Jobs/Fiber.h>

#include <Jobs/FiberRoutines.h>
#include <Jobs/Logging.h>
#include <Jobs/Assert.h>
#include <Jobs/Profiling.h>

#include <utility>  // std::swap

#if JOBS_PLATFORM_WINDOWS
  #include <Jobs/WindowsMinimal.h>
#else
  #include <unistd.h>
  #include <cstdlib>
#endif

namespace Jobs
{
	Fiber::Fiber(size_t StackSize, EntryType Entry, Manager* InOwner) : Data(reinterpret_cast<void*>(InOwner))
	{
		JOBS_SCOPED_STAT("Fiber Creation");

		JOBS_LOG(LogLevel::Log, "Building fiber.");
		JOBS_ASSERT(StackSize > 0, "Stack size must be greater than 0.");

		// Perform a page-aligned allocation for the stack. This is needed to allow for canary pages in overrun detection.

#if JOBS_PLATFORM_WINDOWS
		SYSTEM_INFO sysInfo{};
		GetSystemInfo(&sysInfo);
		const auto alignment = sysInfo.dwPageSize;

		Stack = _aligned_malloc(StackSize, alignment);
#else
		const auto alignment = getpagesize();

		Stack = std::aligned_alloc(alignment, StackSize);
#endif

		void* StackTop = reinterpret_cast<std::byte*>(Stack) + (StackSize * sizeof(std::byte));

		Context = make_fcontext(StackTop, StackSize, Entry);

		JOBS_ASSERT(Context, "Failed to build fiber.");
	}

	Fiber::Fiber(Fiber&& Other) noexcept
	{
		Swap(Other);
	}

	Fiber::~Fiber()
	{
#if JOBS_PLATFORM_WINDOWS
		_aligned_free(Stack);
#else
		std::free(Stack);
#endif
	}

	Fiber& Fiber::operator=(Fiber&& Other) noexcept
	{
		Swap(Other);

		return *this;
	}

	void Fiber::Schedule()
	{
		JOBS_SCOPED_STAT("Fiber Schedule");

		JOBS_LOG(LogLevel::Log, "Scheduling fiber.");

		jump_fcontext(Context, Data);
	}

	void Fiber::Swap(Fiber& Other) noexcept
	{
		std::swap(Context, Other.Context);
		std::swap(Stack, Other.Stack);
		std::swap(Data, Other.Data);
	}
}