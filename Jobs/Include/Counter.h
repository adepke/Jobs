#pragma once

#include <atomic>  // std::atomic
#include <mutex>  // std::mutex
#include <condition_variable>  // std::condition_variable

template <typename T = unsigned int>
class Counter
{
private:
	std::atomic<T> Internal;
	std::mutex Lock;
	std::condition_variable WaitCV;

public:
	Counter();
	Counter(T InitialValue);
	Counter(const Counter&) = delete;
	Counter(Counter&& Other) noexcept;
	~Counter() = default;

	Counter& operator=(const Counter&) = delete;
	Counter& operator=(Counter&&) noexcept = delete;

	Counter& operator++();
	Counter& operator--();

	// Blocking operation.
	void Wait(T ExpectedValue);

	template <typename Rep, typename Period>
	bool WaitFor(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout);
};

template <typename T>
Counter<T>::Counter() : Internal(0) {}

template <typename T>
Counter<T>::Counter(T InitialValue) : Internal(InitialValue) {}

template <typename T>
Counter<T>::Counter(Counter&& Other) noexcept
{
	Internal = std::move(Other.Internal);
}

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

	std::lock_guard LocalLock{ Lock };
	WaitCV.notify_all();

	return *this;
}

template <typename T>
void Counter<T>::Wait(T ExpectedValue)
{
	while (Internal.load() != ExpectedValue)
	{
		std::unique_lock LocalLock{ Lock };
		WaitCV.wait(LocalLock);
	}
}

template <typename T>
template <typename Rep, typename Period>
bool Counter<T>::WaitFor(T ExpectedValue, const std::chrono::duration<Rep, Period>& Timeout)
{
	auto Start{ std::chrono::system_clock::now() };

	while (Internal.load() != ExpectedValue)
	{
		std::unique_lock LocalLock{ Lock };
		auto Status{ WaitCV.wait_for(LocalLock, Timeout) };

		if (Status == std::cv_status::timeout || Start + std::chrono::system_clock::now() >= Timeout)
		{
			return false;
		}
	}

	return true;
}