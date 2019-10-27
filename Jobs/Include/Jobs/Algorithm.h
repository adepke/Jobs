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

	namespace Detail
	{
		constexpr auto PayloadSize = 300;  // Optimized for large data. #TODO: Solve per data set to find the optimal batch size.

		template <typename InIterator, typename UnaryOp, typename OutIterator>
		struct MapPayload
		{
			InIterator Input;
			size_t Count;
			UnaryOp Operation;
			OutIterator Output;
		};

		template <typename Async, typename InIterator, typename OutIterator, typename UnaryOp>
		void ParallelMapInternal(Manager& InManager, InIterator First, InIterator Last, OutIterator Destination, UnaryOp Operation)
		{
			std::shared_ptr<Counter<>> Dependency;

			if constexpr (std::is_same_v<Async, std::false_type>)
			{
				Dependency = std::make_shared<Counter<>>(0);
			}

			const auto Distance = std::distance(First, Last);
			const size_t Count = Distance / PayloadSize;
			const size_t Remainder = Distance % PayloadSize;

			std::vector<MapPayload<InIterator, UnaryOp, OutIterator>> Payloads;
			Payloads.resize(Count);

			// Separate the loops in order to maintain cache locality.
			for (size_t Iter{ 0 }; Iter < Count; ++Iter)
			{
				Payloads[Iter].Input = First + (Iter * PayloadSize);
				Payloads[Iter].Count = PayloadSize;
				Payloads[Iter].Operation = Operation;
				Payloads[Iter].Output = Destination + (Iter * PayloadSize);
			}

			// Fix the remainder.
			Payloads[Count - 1].Count += Remainder;
			
			for (size_t Iter{ 0 }; Iter < Count; ++Iter)
			{
				if constexpr (std::is_same_v<Async, std::true_type>)
				{
					InManager.Enqueue(Job{ [](auto Payload)
						{
							auto* TypedPayload = reinterpret_cast<MapPayload<InIterator, UnaryOp, OutIterator>*>(Payload);

							std::transform(TypedPayload->Input, TypedPayload->Input + TypedPayload->Count, TypedPayload->Output, TypedPayload->Operation);
						}, static_cast<void*>(&Payloads[Iter]) });
				}

				else
				{
					InManager.Enqueue(Job{ [](auto Payload)
						{
							auto* TypedPayload = reinterpret_cast<MapPayload<InIterator, UnaryOp, OutIterator>*>(Payload);

							std::transform(TypedPayload->Input, TypedPayload->Input + TypedPayload->Count, TypedPayload->Output, TypedPayload->Operation);
						}, static_cast<void*>(&Payloads[Iter]) }, Dependency);
				}
			}

			if constexpr (std::is_same_v<Async, std::false_type>)
			{
				Dependency->Wait(0);
			}
		}
	}

	template <typename InIterator, typename OutIterator, typename UnaryOp>
	inline void ParallelMap(Manager& InManager, InIterator First, InIterator Last, OutIterator Destination, UnaryOp Operation)
	{
		return Detail::ParallelMapInternal<std::false_type>(InManager, First, Last, Destination, Operation);
	}

	template <typename Container, typename OutIterator, typename UnaryOp>
	inline void ParallelMap(Manager& InManager, Container&& InContainer, OutIterator Destination, UnaryOp Operation)
	{
		return Detail::ParallelMapInternal<std::false_type>(InManager, std::begin(InContainer), std::end(InContainer), Destination, Operation);
	}

	template <typename InIterator, typename OutIterator, typename UnaryOp>
	inline void ParallelMapAsync(Manager& InManager, InIterator First, InIterator Last, OutIterator Destination, UnaryOp Operation)
	{
		return Detail::ParallelMapInternal<std::true_type>(InManager, First, Last, Destination, Operation);
	}

	template <typename Container, typename OutIterator, typename UnaryOp>
	inline void ParallelMapAsync(Manager& InManager, Container&& InContainer, OutIterator Destination, UnaryOp Operation)
	{
		return Detail::ParallelMapInternal<std::true_type>(InManager, std::begin(InContainer), std::end(InContainer), Destination, Operation);
	}
}