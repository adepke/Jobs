#pragma once

struct Job
{
	using EntryType = void(*)(void* Data);

	EntryType Entry;
	void* Data;
};