#include "TesseraLog.h"

#include <iostream>
#include <ostream>

void TesseraLog::send(const LogType logType, const std::string& title, const std::string& message)
{
    std::cout << "[" << toString(logType) << "] " + title + ": " << message << std::endl;

}

std::string TesseraLog::toString(const LogType logType)
{
	switch (logType)
	{
	case LogType::INFO:
		return "INFO";
	case LogType::DEBUG:
		return "DEBUG";
	case LogType::WARNING:
		return "WARNING";
	case LogType::ERROR:
		return "ERROR";
	}

	return "INFO";
}
