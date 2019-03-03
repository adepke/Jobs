#pragma once

template <typename>
class Counter;

// #TODO
//template <typename ReturnType, typename... ArgTypes>
struct Job
{
	friend class Manager;
	friend void ManagerFiberEntry(void* Data);

	//using EntryType = void(*)(void* Data, ArgTypes...);
	using EntryType = void(*)(void* Data);
	EntryType Entry;

	Job() = default;
	Job(EntryType InEntry) : Entry(InEntry) {}

private:
	void* Data = nullptr;
	std::weak_ptr<Counter<unsigned int>> AtomicCounter;
};