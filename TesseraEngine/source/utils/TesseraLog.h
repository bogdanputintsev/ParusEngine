#pragma once
#include <string>
#include <stdexcept>

namespace tessera
{

	enum class LogType : int
	{
		DEBUG = 1,
		INFO = 2,
		WARNING = 4,
		ERROR = 8,
		FATAL = 16,

		// Presets:
		DEFAULT = INFO | WARNING | ERROR | FATAL,
		ALL = DEBUG | INFO | WARNING | ERROR | FATAL
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

#ifdef NDEBUG
		inline static int logTypeMask = static_cast<int>(LogType::DEFAULT);
#else
		inline static int logTypeMask = static_cast<int>(LogType::ALL);
#endif
		static std::string getLogMessage(const LogType logType, const std::string& filename, const long line, const std::string& message);
		static bool isLogTypeEnabled(const LogType logType) { return static_cast<int>(logType) & logTypeMask; }
		static std::string toString(LogType logType);
	};

#if _WIN32
#define T_FILENAME (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define T_FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#define DEBUG(msg) tessera::TesseraLog::send(tessera::LogType::DEBUG, T_FILENAME, __LINE__, msg)
#define INFO(msg) tessera::TesseraLog::send(tessera::LogType::INFO, T_FILENAME, __LINE__, msg)
#define WARNING(msg) tessera::TesseraLog::send(tessera::LogType::WARNING, T_FILENAME, __LINE__, msg)
#define ERROR(msg) tessera::TesseraLog::send(tessera::LogType::ERROR, T_FILENAME, __LINE__, msg)
#define FATAL(msg) tessera::TesseraLog::send(tessera::LogType::FATAL, T_FILENAME, __LINE__, msg)
#define LOG(type, msg) tessera::TesseraLog::send(type, T_FILENAME, __LINE__, msg)

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
            FATAL(msg);											\
			throw std::runtime_error("Assertion failed.");		\
        }														\
    } while (0)

#define REGISTER_EVENT(type, func) tessera::registerHelper(*CORE->eventSystem, type, func)
#define FIRE_EVENT(type, args, ...) CORE->eventSystem->fireEvent(type, args, __VA_ARGS__)

}

