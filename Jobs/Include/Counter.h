#pragma once

#include <atomic>  // std::atomic
#include <mutex>  // std::mutex
#include "Futex.h"
#include <condition_variable>  // std::condition_variable

template <typename T = unsigned int>
class Counter
{
	friend void ManagerFiberEntry(void*);

public:
	using Type = T;

private:
	std::atomic<T> Internal;
	std::mutex KernelLock;
	Futex UserSpaceLock;
	std::condition_variable WaitCV;

	bool Evaluate(const T& ExpectedValue) const
	{
		return Internal.load() <= ExpectedValue;  // #TODO: Memory order.
	}

public:
	Counter();
	Counter(T InitialValue);
	Counter(const Counter&) = delete;
	Counter(Counter&& Other) noexcept = delete;  // #TODO: Implement.
	~Counter() = default;

	Counter& operator=(const Counter&) = delete;
	Counter& operator=(Counter&&) noexcept = delete;  // #TODO: Implement.

	Counter& operator++();
	Counter& operator--();

	// Atomically fetch the current value.
	const T Get() const;

	// Blocking operation.
	void Wait(T ExpectedValue);

	// Blocking operation.
	template <typename Rep, typename Period>
	bool WaitFor(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout);

protected:
	// Futex based blocking, reserved for jobs.
	template <typename Rep, typename Period>
	bool WaitUserSpace(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout);
};

template <typename T>
Counter<T>::Counter() : Internal(0) {}

template <typename T>
Counter<T>::Counter(T InitialValue) : Internal(InitialValue) {}

template <typename T>
Counter<T>& Counter<T>::operator++()
{
	++Internal;

	// Don't notify.

	return *this;
}

template <typename T>
Counter<T>& Counter<T>::operator--()
{
	--Internal;

	// Notify the user space lock.
	UserSpaceLock.NotifyAll();

	// Notify the kernel lock.
	std::lock_guard LocalLock{ KernelLock };
	WaitCV.notify_all();

	return *this;
}

template <typename T>
const T Counter<T>::Get() const
{
	return Internal.load();  // #TODO: Memory order.
}

template <typename T>
void Counter<T>::Wait(T ExpectedValue)
{
	while (!Evaluate(ExpectedValue))
	{
		std::unique_lock LocalLock{ KernelLock };
		WaitCV.wait(LocalLock);
	}
}

template <typename T>
template <typename Rep, typename Period>
bool Counter<T>::WaitFor(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout)
{
	auto Start{ std::chrono::system_clock::now() };

	while (!Evaluate(ExpectedValue))
	{
		std::unique_lock LocalLock{ KernelLock };
		auto Status{ WaitCV.wait_for(LocalLock, Timeout) };

		if (Status == std::cv_status::timeout || Start + std::chrono::system_clock::now() >= Timeout)
		{
			return false;
		}
	}

	return true;
}

template <typename T>
template <typename Rep, typename Period>
bool Counter<T>::WaitUserSpace(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout)
{
	// InternalCapture is the saved state of Internal at the time of sleeping. We will use this to know if it changed.
	if (auto InternalCapture{ Internal.load() }; InternalCapture != ExpectedValue)  // #TODO: Memory order.
	{
		UserSpaceLock.Set(&Internal);

		auto TimeRemaining{ Timeout };  // Used to represent the time budget of the sleep operation. Changes.
		auto Start{ std::chrono::system_clock::now() };

		// The counter can change multiple times during our allocated timeout period, so we need to loop until we either timeout of successfully met expected value.
		while (true)
		{
			if (UserSpaceLock.Wait(&InternalCapture, TimeRemaining))
			{
				// Value changed, did not time out. Re-evaluate and potentially try again if we have time.

				if (Evaluate(ExpectedValue))
				{
					// Success.
					return true;
				}

				// Didn't meet the requirement, fall through.
			}

			// Timed out.
			else
			{
				break;
			}

			// Attempt to reschedule a sleep.
			auto Remainder{ Timeout - (std::chrono::system_clock::now() - Start) };
			if (Remainder.count() > 0)
			{
				// We have time for another, prepare for the upcoming sleep.

				InternalCapture = Internal.load();  // #TODO: Memory order.
				TimeRemaining = std::chrono::round<decltype(TimeRemaining)>(Remainder);  // Update the time budget.
			}

			else
			{
				// Spent our time budget, fail out.
				return false;
			}
		}

		return false;
	}

	return true;
}