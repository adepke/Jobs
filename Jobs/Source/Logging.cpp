#include "../Include/Logging.h"

#include <cstddef>  // std::size_t
#include <cstdarg>  // std::va_list, va_start, va_end
#include <cstdio>  // std::vsnprintf

void LogManager::Log(LogLevel Level, const std::string& Format, ...)
{
	if (!OutputDevice)
	{
		return;
	}

	std::string Output;

	switch (Level)
	{
	case LogLevel::Log:
		Output = "[Log] ";
		break;
	case LogLevel::Warning:
		Output = "[Warning] ";
		break;
	case LogLevel::Error:
		Output = "[Error] ";
		break;
	}

	constexpr size_t BufferSize = 1024;
	char Buffer[BufferSize];

	std::va_list Args;
	va_start(Args, &Format);
	std::vsnprintf(Buffer, BufferSize, Format.c_str(), Args);
	va_end(Args);

	std::string Formatted{ Buffer };

	Output += Formatted + "\n";

	*OutputDevice << Output.c_str();
}

void LogManager::SetOutputDevice(std::ostream& Device)
{
	OutputDevice = &Device;
}