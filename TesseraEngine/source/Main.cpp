#include <iostream>

#include "Core.h"
#include "Application.h"
#include "utils/TesseraLog.h"

// FIXME: Fix issue with shaders directory.

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
		FATAL(exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
