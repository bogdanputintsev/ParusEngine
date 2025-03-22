#include "TesseraLog.h"

#include <iomanip>
#include <iostream>
#include <sstream>

namespace tessera
{
	void TesseraLog::send(const LogType logType, const std::string& filename, const long line, const std::string& message)
	{
		if (!isLogTypeEnabled(logType))
		{
			return;
		}

		const std::string logMessage = getLogMessage(logType, filename, line, message);

		// Print out to the console.
		std::cout << logMessage << '\n';
	}

	std::string TesseraLog::getLogMessage(const LogType logType, const std::string& filename, const long line, const std::string& message)
	{
		// FIXME: __TIME__ macro shows time when it was compiled. Not the current time.
		std::stringstream buffer;
		buffer << __DATE__ << " " << __TIME__ << " "
			<< toString(logType) << " "
			<< "[" << filename << ":" << line << "] - "
			<< message;

		return buffer.str();
	}

	std::string TesseraLog::toString(const LogType logType)
	{
		switch (logType)
		{
			using enum LogType;
			case INFO:		return "INFO";
			case DEBUG:		return "DEBUG";
			case WARNING:	return "WARNING";
			case TE_ERROR:		return "ERROR";
			case FATAL:		return "FATAL";
			case ALL:		break;
			case DEFAULT:	break;
		}

		return "INFO";
	}
}
