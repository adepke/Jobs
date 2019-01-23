#pragma once

struct Job
{
	using EntryType = void(*)(void* Data);

	EntryType Entry;
	void* Data = nullptr;

	/*
	Job() {}
	Job(const Job&) {}
	Job(Job&&) noexcept {}
	Job& operator=(Job&&) noexcept { return *this; }
	Job& operator=(const Job&) { return *this; }
	*/
};