#pragma once

#include "Counter.h"

struct Job
{
	using EntryType = void(*)(void* Data);

	EntryType Entry;
	void* Data = nullptr;
	Counter<>* AtomicCounter = nullptr;
};