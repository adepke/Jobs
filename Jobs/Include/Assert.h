#pragma once

#include <iostream>  // std::cerr
#include <exception>  // std::terminate

// TEMP
#undef _DEBUG

#if _DEBUG || NDEBUG
#define JOBS_ASSERT(Expression, ...) \
	do \
	{ \
		if (!Expression) \
		{ \
			std::cerr << "Assertion \"" << #Expression << "\" failed in " << __FILE__ << ", line " << __LINE__ __VA_OPT__(<< ": " << __VA_ARGS__); \
			std::terminate(); \
		} \
	} \
	while (0)
#else
#define JOBS_ASSERT(Expression, ...) do {} while (0)
#endif