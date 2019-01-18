#include "../Include/Fiber.h"

#include "../Include/Logging.h"
#include "../Include/Assert.h"

#include <utility>  // std::swap

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
#define NOVIRTUALKEYCODES
#define NOWINMESSAGES
#define NOWINSTYLES
#define NOSYSMETRICS
#define NOMENUS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
#define NORASTEROPS
#define NOSHOWWINDOW
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NODRAWTEXT
#define NOGDI
#define NOKERNEL
#define NOUSER
#define NONLS
#define NOMB
#define NOMEMMGR
#define NOMETAFILE
#define NOMINMAX
#define NOMSG
#define NOOPENFILE
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOTEXTMETRIC
#define NOWH
#define NOWINOFFSETS
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX
#include <Windows.h>
#else
#define PLATFORM_POSIX 1
#include <ucontext.h>
#endif

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_POSIX
#define PLATFORM_POSIX 0
#endif

void FiberEntry(void* Data)
{
	JOBS_LOG("Fiber entry.");
}

Fiber::Fiber(void* Arg) : Data(Arg) {}

Fiber::Fiber(std::size_t StackSize, decltype(&FiberEntry) Entry, void* Arg) : Data(Arg)
{
	JOBS_LOG("Building fiber.");
	JOBS_ASSERT(StackSize > 0, "Stack size must be greater than 0.");

#if PLATFORM_WINDOWS
	Context = CreateFiber(StackSize, Entry, Arg);
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
	JOBS_LOG("Destroying fiber.");
	JOBS_ASSERT(Context, "Fiber had no execution context.");

#if PLATFORM_WINDOWS
	DeleteFiber(Context);
#else
	delete[] Context->uc_stack.ss_sp;
	delete Context;
#endif
}

Fiber& Fiber::operator=(Fiber&& Other) noexcept
{
	Swap(Other);

	return *this;
}

void Fiber::Schedule(const Fiber& From)
{
	JOBS_LOG("Scheduling fiber.");

#if PLATFORM_WINDOWS
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
	std::swap(Data, Other.Data);
}

std::shared_ptr<Fiber> Fiber::FromThisThread(void* Arg)
{
	auto Result{ std::make_shared<Fiber>(Arg) };

#if PLATFORM_WINDOWS
	Result->Context = ConvertThreadToFiber(Arg);
#else
	Result->Context = new ucontext_t{};
	JOBS_ASSERT(getcontext(Result->Context) == 0, "Error occurred in getcontext().");
#endif

	return Result;
}