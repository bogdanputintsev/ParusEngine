#include "TesseraLog.h"

#include <iostream>
#include <ostream>

namespace tessera
{
	void TesseraLog::send(const LogType logType, const std::string& title, const std::string& message)
	{
		if (!isLogTypeEnabled(logType))
		{
			return;
		}

	    std::cout << "[" << toString(logType) << "] " + title + ": " << message << std::endl;
	}

	std::string_view TesseraLog::toString(const LogType logType)
	{
		switch (logType)
		{
			using enum LogType;
			case INFO:		return "INFO";
			case DEBUG:		return "DEBUG";
			case WARNING:	return "WARNING";
			case ERROR:		return "ERROR";
			case FATAL:		return "FATAL";
			case ALL:		break;
			case DEFAULT:	break;
		}

		return "INFO";
	}
}
