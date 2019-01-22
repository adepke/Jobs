#pragma once

#include <ostream>

#include "Spinlock.h"

#ifndef JOBS_DISABLE_LOGGING
#define JOBS_LOG(Level, Format, ...) LogManager::Get().Log(Level, Format, ## __VA_ARGS__)
#else
#define JOBS_LOG(Level, Format, ...) do {} while (0)
#endif

enum class LogLevel
{
	Log,
	Warning,
	Error
};

class LogManager
{
private:
	Spinlock Lock;

	std::ostream* OutputDevice = nullptr;

public:
	LogManager() = default;
	LogManager(const LogManager&) = delete;
	LogManager(LogManager&&) noexcept = delete;
	LogManager& operator=(const LogManager&) = delete;
	LogManager& operator=(LogManager&&) noexcept = delete;

	static inline LogManager& Get()
	{
		static LogManager Singleton;

		return Singleton;
	}

	void Log(LogLevel Level, const std::string& Format, ...);

	void SetOutputDevice(std::ostream& Device);
};