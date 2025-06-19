#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Core.h"
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
		FATAL(exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
