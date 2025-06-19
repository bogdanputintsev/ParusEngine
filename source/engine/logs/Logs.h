#pragma once
#include <string>
#include <stdexcept>

#include "engine/Defines.h"

namespace parus
{

	enum class LogType : int
	{
		LOG_TYPE_DEBUG = 1,
		LOG_TYPE_INFO = 2,
		LOG_TYPE_WARNING = 4,
		LOG_TYPE_ERROR = 8,
		LOG_TYPE_FATAL = 16,

		// Presets:
		DEFAULT = LOG_TYPE_INFO | LOG_TYPE_WARNING | LOG_TYPE_ERROR | LOG_TYPE_FATAL,
		ALL = LOG_TYPE_DEBUG | LOG_TYPE_INFO | LOG_TYPE_WARNING | LOG_TYPE_ERROR | LOG_TYPE_FATAL
	};

	inline LogType operator|(LogType a, LogType b)
	{
		return static_cast<LogType>(static_cast<int>(a) | static_cast<int>(b));
	}

	class Logs final
	{
	public:
		static void send(const LogType logType, const std::string& filename, const long line, const std::string& message);
		
	private:
#if IN_DEBUG_MODE
		inline static int logTypeMask = static_cast<int>(LogType::ALL);
#else
		inline static int logTypeMask = static_cast<int>(LogType::DEFAULT);
#endif
		
		static std::string getLogMessage(const LogType logType, const std::string& filename, const long line, const std::string& message);
		static std::string getCurrentDateTime();
		static bool isLogTypeEnabled(const LogType logType) { return static_cast<int>(logType) & logTypeMask; }
		static std::string toString(LogType logType);
	};

#define LOG_DEBUG(msg) ::parus::Logs::send(::parus::LogType::LOG_TYPE_DEBUG, GET_FILENAME, __LINE__, msg)
#define LOG_INFO(msg) ::parus::Logs::send(::parus::LogType::LOG_TYPE_INFO, GET_FILENAME, __LINE__, msg)
#define LOG_WARNING(msg) ::parus::Logs::send(::parus::LogType::LOG_TYPE_WARNING, GET_FILENAME, __LINE__, msg)
#define LOG_ERROR(msg) ::parus::Logs::send(::parus::LogType::LOG_TYPE_ERROR, GET_FILENAME, __LINE__, msg)
#define LOG_FATAL(msg) ::parus::Logs::send(::parus::LogType::LOG_TYPE_FATAL, GET_FILENAME, __LINE__, msg)
#define LOG(type, msg) ::parus::Logs::send(type, GET_FILENAME, __LINE__, msg)

}

