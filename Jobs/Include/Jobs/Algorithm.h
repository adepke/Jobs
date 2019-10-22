// Copyright (c) 2019 Andrew Depke

#pragma once

#include <Jobs/Manager.h>

namespace Jobs
{
	namespace Detail
	{
		struct Empty;  // Forward declare, we need it in the algorithm payload.
	}

	template <typename... Ts>
	struct AlgorithmPayload;

	template <typename Container>
	struct AlgorithmPayload<Container, Detail::Empty>
	{
		decltype(std::declval<Container>().begin()) Iterator;
	};

	template <typename Container, typename CustomData>
	struct AlgorithmPayload<Container, CustomData>
	{
		decltype(std::declval<Container>().begin()) Iterator;
		CustomData Data;
	};

	namespace Detail
	{
		template <typename Iterator>
		struct FakeContainer
		{
			Iterator begin() const;
		};

		struct Empty {};

		template <typename Async, typename Iterator, typename CustomData, typename Function>
		void ParallelForInternal(Manager& InManager, Iterator First, Iterator Last, CustomData&& Data, Function InFunction)
		{
			std::shared_ptr<Counter<>> Dependency;

			if constexpr (std::is_same_v<Async, std::false_type>)
			{
				Dependency = std::make_shared<Counter<>>(0);
			}

			std::vector<AlgorithmPayload<Detail::FakeContainer<Iterator>, std::remove_reference_t<CustomData>>> Payloads;
			Payloads.resize(std::distance(First, Last));
			static_assert(std::is_trivially_copyable_v<AlgorithmPayload<Detail::FakeContainer<Iterator>, std::remove_reference_t<CustomData>>>, "AlgorithmPayload must be trivially copyable");

			if constexpr (!std::is_same_v<CustomData, Detail::Empty>)
			{
				std::fill(Payloads.begin(), Payloads.end(), AlgorithmPayload<Detail::FakeContainer<Iterator>, std::remove_reference_t<CustomData>>{ First, Data });
			}

			const auto CachedFirst = First;

			for (; First != Last; ++First)
			{
				const auto Index = std::distance(CachedFirst, First);
				Payloads[Index].Iterator = First;

				if constexpr (std::is_same_v<Async, std::true_type>)
				{
					InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&Payloads[Index])) });
				}

				else
				{
					InManager.Enqueue(Job{ InFunction, const_cast<void*>(static_cast<const void*>(&Payloads[Index])) }, Dependency);
				}
			}

			if constexpr (std::is_same_v<Async, std::false_type>)
			{
				Dependency->Wait(0);
			}
		}
	}

	template <typename Iterator, typename CustomData, typename Function>
	inline void ParallelFor(Manager& InManager, Iterator First, Iterator Last, CustomData&& Data, Function InFunction)
	{
		Detail::ParallelForInternal<std::false_type>(InManager, First, Last, std::forward<CustomData>(Data), InFunction);
	}

	template <typename Iterator, typename Function>
	inline void ParallelFor(Manager& InManager, Iterator First, Iterator Last, Function InFunction)
	{
		Detail::ParallelForInternal<std::false_type>(InManager, First, Last, Detail::Empty{}, InFunction);
	}

	template <typename Container, typename CustomData, typename Function>
	inline void ParallelFor(Manager& InManager, Container&& InContainer, CustomData&& Data, Function InFunction)
	{
		Detail::ParallelForInternal<std::false_type>(InManager, std::begin(InContainer), std::end(InContainer), std::forward<CustomData>(Data), InFunction);
	}

	template <typename Container, typename Function>
	inline void ParallelFor(Manager& InManager, Container&& InContainer, Function InFunction)
	{
		Detail::ParallelForInternal<std::false_type>(InManager, std::begin(InContainer), std::end(InContainer), Detail::Empty{}, InFunction);
	}

	template <typename Iterator, typename CustomData, typename Function>
	inline void ParallelForAsync(Manager& InManager, Iterator First, Iterator Last, CustomData&& Data, Function InFunction)
	{
		Detail::ParallelForInternal<std::true_type>(InManager, First, Last, std::forward<CustomData>(Data), InFunction);
	}
	
	template <typename Iterator, typename Function>
	inline void ParallelForAsync(Manager& InManager, Iterator First, Iterator Last, Function InFunction)
	{
		Detail::ParallelForInternal<std::true_type>(InManager, First, Last, Detail::Empty{}, InFunction);
	}

	template <typename Container, typename CustomData, typename Function>
	inline void ParallelForAsync(Manager& InManager, Container&& InContainer, CustomData&& Data, Function InFunction)
	{
		Detail::ParallelForInternal<std::true_type>(InManager, std::begin(InContainer), std::end(InContainer), std::forward<CustomData>(Data), InFunction);
	}

	template <typename Container, typename Function>
	inline void ParallelForAsync(Manager& InManager, Container&& InContainer, Function InFunction)
	{
		Detail::ParallelForInternal<std::true_type>(InManager, std::begin(InContainer), std::end(InContainer), Detail::Empty{}, InFunction);
	}
}