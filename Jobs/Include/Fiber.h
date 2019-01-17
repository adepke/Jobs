#pragma once

#include <cstddef>  // std::size_t
#include <memory>  // std::shared_ptr

void FiberEntry(void* Data);

class Fiber
{
private:
	void* NativeHandle = nullptr;
	void* Data = nullptr;

public:
	Fiber(void* Arg);  // Used for converting a thread to a fiber.
	Fiber(std::size_t StackSize, decltype(&FiberEntry) Entry, void* Arg);
	Fiber(const Fiber&) = delete;
	Fiber(Fiber&& Other) noexcept;
	~Fiber();

	Fiber& operator=(const Fiber&) = delete;
	Fiber& operator=(Fiber&& Other) noexcept;

	void Schedule();

	void Swap(Fiber& Other) noexcept;

	static std::shared_ptr<Fiber> FromThisThread(void* Arg);
};