#include <iostream>

#include "Application.h"
#include "utils/TesseraLog.h"

int main()
{
	tessera::Application app{};

    try
    {
        app.init();
        app.clean();
    }
    catch(const std::exception& exception)
    {
        // TODO: Create custom exception with title.
        tessera::TesseraLog::send(tessera::LogType::FATAL, "", exception.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
