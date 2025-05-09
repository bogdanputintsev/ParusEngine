#pragma once
#include <string>
#include <stdexcept>

#include "core/Defines.h"

namespace tessera
{

	enum class LogType : int
	{
		DEBUG = 1,
		INFO = 2,
		WARNING = 4,
		TE_ERROR = 8,
		FATAL = 16,

		// Presets:
		DEFAULT = INFO | WARNING | TE_ERROR | FATAL,
		ALL = DEBUG | INFO | WARNING | TE_ERROR | FATAL
	};

	inline LogType operator|(LogType a, LogType b)
	{
		return static_cast<LogType>(static_cast<int>(a) | static_cast<int>(b));
	}

	class TesseraLog final
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
		static bool isLogTypeEnabled(const LogType logType) { return static_cast<int>(logType) & logTypeMask; }
		static std::string toString(LogType logType);
	};

#if WITH_WINDOWS_PLATFORM
	#define GET_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
	#define GET_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define LOG_DEBUG(msg) ::tessera::TesseraLog::send(::tessera::LogType::DEBUG, GET_FILENAME, __LINE__, msg)
#define LOG_INFO(msg) ::tessera::TesseraLog::send(::tessera::LogType::INFO, GET_FILENAME, __LINE__, msg)
#define LOG_WARNING(msg) ::tessera::TesseraLog::send(::tessera::LogType::WARNING, GET_FILENAME, __LINE__, msg)
#define LOG_ERROR(msg) ::tessera::TesseraLog::send(::tessera::LogType::TE_ERROR, GET_FILENAME, __LINE__, msg)
#define LOG_FATAL(msg) ::tessera::TesseraLog::send(::tessera::LogType::FATAL, GET_FILENAME, __LINE__, msg)
#define LOG(type, msg) ::tessera::TesseraLog::send(type, GET_FILENAME, __LINE__, msg)

/**
 * \brief This macro assures that the condition is true. Otherwise, it logs an error and throw an exception.
 * \param condition The boolean condition that will be checked.
 * \param msg This message will be printed if the condition fails.
 */
#define ASSERT(condition, msg)									\
    do															\
	{															\
        if (!(condition))										\
		{														\
            LOG_FATAL(msg);										\
			throw std::runtime_error("Assertion failed.");		\
        }														\
    } while (0)

/**
 * \brief This macro assures that the condition is true if application is in debug mode.
 * Otherwise, it logs an error and throw an exception.
 * \param condition The boolean condition that will be checked.
 * \param msg This message will be printed if the condition fails.
 */
#if IN_DEBUG_MODE												
	#define DEBUG_ASSERT(condition, msg) ASSERT(condition, msg)									
#else
	#define DEBUG_ASSERT(condition, msg)
#endif
    	
#define REGISTER_EVENT(type, func) tessera::registerHelper(*CORE->eventSystem, type, func)
#define FIRE_EVENT(type, args, ...) CORE->eventSystem->fireEvent(type, args, __VA_ARGS__)

}

