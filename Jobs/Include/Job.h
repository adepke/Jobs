#pragma once

#include <memory>  // std::weak_ptr
#include <vector>  // std::vector
#include <utility>  // std::pair, std::move
#include "Counter.h"

class Job
{
	friend class Manager;
	friend void ManagerFiberEntry(void* Data);

private:
	void* Data = nullptr;
	std::weak_ptr<Counter<>> AtomicCounter;

	// List of dependencies this job needs before executing. Pairs of counters to expected values.
	std::vector<std::pair<std::weak_ptr<Counter<>>, Counter<>::Type>> Dependencies;

public:
	using EntryType = void(*)(void* Data);
	EntryType Entry;

	Job() = default;
	Job(EntryType InEntry, void* InData = nullptr) : Entry(InEntry), Data(InData) {}

	void AddDependency(const std::shared_ptr<Counter<>>& Handle, const Counter<>::Type ExpectedValue)
	{
		Dependencies.push_back({ Handle, ExpectedValue });
	}
};