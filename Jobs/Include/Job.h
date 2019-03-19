#pragma once

#include "Kernel.h"
#include <memory>  // std::shared_ptr, std::weak_ptr
#include <vector>  // std::vector
#include <utility>  // std::pair
#include "Counter.h"
#include "Assert.h"

class Job
{
	friend class Manager;
	friend void ManagerFiberEntry(void* Data);

private:
	Kernel<void(void*)> Kern;

	void* Data = nullptr;
	std::weak_ptr<Counter<>> AtomicCounter;

	// List of dependencies this job needs before executing. Pairs of counters to expected values.
	std::vector<std::pair<std::weak_ptr<Counter<>>, Counter<>::Type>> Dependencies;

public:
	Job() = default;
	Job(decltype(Kern) InKernel, void* InData = nullptr) : Kern(std::move(InKernel)), Data(InData) {}

	void AddDependency(const std::shared_ptr<Counter<>>& Handle, const Counter<>::Type ExpectedValue)
	{
		Dependencies.push_back({ Handle, ExpectedValue });
	}

	void operator()()
	{
		return Kern(Data);
	}
};