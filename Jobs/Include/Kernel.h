#pragma once

#include "Assert.h"

template <typename>
struct Kernel;

template <typename Input, typename Output>
struct Kernel<Output(Input)>
{
	static constexpr size_t Size = 32;

private:
	char Buffer[Size];
	bool Valid = false;

	struct Concept
	{
		virtual Output operator()(Input In) = 0;
		virtual ~Concept() = default;
	};

	template <typename Functor>
	struct Model final : public Concept
	{
		Functor Internal;

		Model(const Functor& InFunctor) : Internal(InFunctor) {}

		virtual Output operator()(Input In) override
		{
			return Internal(In);
		}
	};

public:
	Kernel() = default;

	template <typename Functor>
	Kernel(const Functor& InFunctor)
	{
		static_assert(sizeof(InFunctor) <= Size, "Job kernel functor cannot exceed set size");

		new(Buffer) Model<Functor>{ InFunctor };
	}

	Kernel(const Kernel& Other)
	{
		std::memcpy(Buffer, Other.Buffer, Size);
		Valid = Other.Valid;
	}

	Kernel(Kernel&& Other) noexcept
	{
		std::swap(Buffer, Other.Buffer);
		std::swap(Valid, Other.Valid);
	}

	Kernel& operator=(const Kernel& Other)
	{
		if (Valid)
		{
			((Concept*)Buffer)->~Concept();
		}

		std::memcpy(Buffer, Other.Buffer, Size);
		Valid = Other.Valid;

		return *this;
	}

	Kernel& operator=(Kernel&& Other) noexcept
	{
		std::swap(Buffer, Other.Buffer);
		std::swap(Valid, Other.Valid);

		return *this;
	}

	Output operator()(Input In)
	{
		JOBS_ASSERT(Valid, "Attempted to execute empty job kernel.");

		return ((Concept*)Buffer)->operator()(In);
	}

	~Kernel()
	{
		if (Valid)
		{
			((Concept*)Buffer)->~Concept();
		}
	}
};