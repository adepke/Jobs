#pragma once

#include <ostream>

#ifndef JOBS_DISABLE_LOGGING
#define JOBS_LOG(Message, ...) LogManager::Get().Log(Message, ## __VA_ARGS__)
#else
#define JOBS_LOG(Message, ...) do {} while (0)
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

	void Log(const std::string& Message, LogLevel Level = LogLevel::Log);

	void SetOutputDevice(std::ostream& Device);
};