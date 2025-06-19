#include "Logs.h"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "engine/Asserts.h"

namespace parus
{
	void Logs::send(const LogType logType, const std::string& filename, const long line, const std::string& message)
	{
		if (!isLogTypeEnabled(logType))
		{
			return;
		}

		const std::string logMessage = getLogMessage(logType, filename, line, message);

		// Print out to the console.
		printf("%s\n", logMessage.c_str());
	}

	std::string Logs::getLogMessage(const LogType logType, const std::string& filename, const long line, const std::string& message)
	{
		std::stringstream buffer;
		
		buffer
			<< getCurrentDateTime() << " "
			<< toString(logType) << " "
			<< "[" << filename << ":" << line << "] - "
			<< message;

		return buffer.str();
	}

	std::string Logs::getCurrentDateTime()
	{
		const auto now = std::chrono::system_clock::now();
		const std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
		std::tm timeInfo{};
#ifdef WITH_WINDOWS_PLATFORM
		ASSERT(localtime_s(&timeInfo, &currentTime) == 0, "Failed to get current time.");
#else
		localtime_r(&now_c, &timeInfo);  // POSIX secure version
#endif

		std::ostringstream ss;
		ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
		return ss.str();
	}

	std::string Logs::toString(const LogType logType)
	{
		switch (logType)
		{
			using enum LogType;
			case LOG_TYPE_INFO:		return "INFO";
			case LOG_TYPE_DEBUG:	return "DEBUG";
			case LOG_TYPE_WARNING:	return "WARNING";
			case LOG_TYPE_ERROR:	return "ERROR";
			case LOG_TYPE_FATAL:	return "FATAL";
			case ALL:				break;
			case DEFAULT:			break;
		}

		return "INFO";
	}
}
