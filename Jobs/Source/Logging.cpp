#include "../Include/Logging.h"

void LogManager::Log(const std::string& Message, LogLevel Level /*= LogLevel::Log*/)
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

	Output += Message + "\n";

	*OutputDevice << Output.c_str();
}

void LogManager::SetOutputDevice(std::ostream& Device)
{
	OutputDevice = &Device;
}