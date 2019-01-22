#pragma once

struct Job
{
public:
	Job() {}
	~Job() {}

	virtual void Execute() = 0;
};