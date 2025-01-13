#include <iostream>

#include "Application.h"
#include "utils/TesseraLog.h"

int main()
{
	try
	{
		tessera::Application::init();
		tessera::Application::loop();
		tessera::Application::clean();
	}
	catch (const std::exception& exception)
	{
		// TODO: Create custom exception with title.
		tessera::TesseraLog::send(tessera::LogType::FATAL, "", exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
