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
#define PLATFORM_UNIX 1

#endif

#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 0
#endif
#ifndef PLATFORM_UNIX
#define PLATFORM_UNIX 0
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
	NativeHandle = CreateFiber(StackSize, Entry, Arg);
#else

#endif
}

Fiber::Fiber(Fiber&& Other) noexcept
{
	Swap(Other);
}

Fiber::~Fiber()
{
	JOBS_LOG("Destroying fiber.");
	JOBS_ASSERT(NativeHandle, "Fiber had no execution context.");

#if PLATFORM_WINDOWS
	DeleteFiber(NativeHandle);
#else
#endif
}

Fiber& Fiber::operator=(Fiber&& Other) noexcept
{
	Swap(Other);

	return *this;
}

void Fiber::Schedule()
{
	JOBS_LOG("Scheduling fiber.");

#if PLATFORM_WINDOWS
	JOBS_ASSERT(NativeHandle != GetCurrentFiber(), "Fibers scheduling themselves causes unpredictable issues.");

	SwitchToFiber(NativeHandle);
#else
#endif
}

void Fiber::Swap(Fiber& Other) noexcept
{
	std::swap(NativeHandle, Other.NativeHandle);
	std::swap(Data, Other.Data);
}

std::shared_ptr<Fiber> Fiber::FromThisThread(void* Arg)
{
	auto Result{ std::make_shared<Fiber>(Arg) };

#if PLATFORM_WINDOWS
	Result->NativeHandle = ConvertThreadToFiber(Arg);
#else
#endif

	return Result;
}