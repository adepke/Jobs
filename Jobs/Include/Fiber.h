#pragma once

#include <atomic>  // std::atomic_flag
#include <cstddef>  // std::size_t
#include <memory>  // std::shared_ptr

void FiberEntry(void* Data);

class Fiber
{
private:
	void* Context = nullptr;
	void* Data = nullptr;

	std::atomic_bool Executing = false;

public:
	Fiber(void* Arg);  // Used for converting a thread to a fiber.
	Fiber(std::size_t StackSize, decltype(&FiberEntry) Entry, void* Arg);
	Fiber(const Fiber&) = delete;
	Fiber(Fiber&& Other) noexcept;
	~Fiber();

	Fiber& operator=(const Fiber&) = delete;
	Fiber& operator=(Fiber&& Other) noexcept;

	void Schedule(Fiber& From);

	void Swap(Fiber& Other) noexcept;

	bool IsExecuting() const;

	static std::shared_ptr<Fiber> FromThisThread(void* Arg);
};