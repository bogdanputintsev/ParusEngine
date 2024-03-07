#pragma once
#include <string>

// TODO: Add timestamp to log message.

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
		ALL =  DEBUG | INFO | WARNING | ERROR | FATAL
	};

	inline LogType operator|(LogType a, LogType b)
	{
		return static_cast<LogType>(static_cast<int>(a) | static_cast<int>(b));
	}

	class TesseraLog final
	{
	public:
		static void send(LogType logType, const std::string& title, const std::string& message);
	private:

#ifdef NDEBUG
		inline static int logTypeMask = static_cast<int>(LogType::DEFAULT);
#else
		inline static int logTypeMask = static_cast<int>(LogType::ALL);
#endif

		static bool isLogTypeEnabled(const LogType logType) { return static_cast<int>(logType) & logTypeMask; }
		static std::string_view toString(LogType logType);
	};

}

