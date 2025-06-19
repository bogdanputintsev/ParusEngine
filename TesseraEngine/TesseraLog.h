#pragma once
#include <string>

enum class LogType : int
{
	INFO = 0,
	DEBUG = 1,
	WARNING = 2,
	ERROR = 3
};

class TesseraLog final
{
public:
	static void send(LogType logType, const std::string& title, const std::string& message);
private:
	int logTypeMask = LogType::DEBUG | LogType::INFO;
	static std::string toString(LogType logType);
};

