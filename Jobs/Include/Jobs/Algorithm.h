// Copyright (c) 2019 Andrew Depke

#pragma once

#include <Jobs/Manager.h>

namespace Jobs
{
	template <typename Iterator, typename Function>
	void ParallelFor(Manager& InManager, Iterator First, Iterator Last, Function InFunction)
	{
		std::shared_ptr<Counter<>> Dependency = std::make_shared<Counter<>>(0);

		for (; First != Last; ++First)
		{
			InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&*First)) }, Dependency);
		}

		Dependency->Wait(0);
	}

	template <typename Container, typename Function>
	void ParallelFor(Manager& InManager, Container&& InContainer, Function InFunction)
	{
		std::shared_ptr<Counter<>> Dependency = std::make_shared<Counter<>>(0);

		auto First{ std::begin(InContainer) };
		const auto Last{ std::end(InContainer) };

		for (; First != Last; ++First)
		{
			InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&*First)) }, Dependency);
		}

		Dependency->Wait(0);
	}

	template <typename Iterator, typename Function>
	void ParallelForAsync(Manager& InManager, Iterator First, Iterator Last, Function InFunction)
	{
		for (; First != Last; ++First)
		{
			InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&*First)) });
		}
	}

	template <typename Container, typename Function>
	void ParallelForAsync(Manager& InManager, Container&& InContainer, Function InFunction)
	{
		auto First{ std::begin(InContainer) };
		const auto Last{ std::end(InContainer) };

		for (; First != Last; ++First)
		{
			InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&*First)) });
		}
	}
}