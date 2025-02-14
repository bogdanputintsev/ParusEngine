#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "Core.h"
#include "Application.h"
#include "utils/TesseraLog.h"

int main()
{
	tessera::Application application;
	
	try
	{
		application.init();
		application.loop();
		application.clean();
	}
	catch (const std::exception& exception)
	{
		LOG_FATAL(exception.what());
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
