#include <iostream>

#include "Application.h"

int main()
{
	tessera::Application app{};

    try
    {
        app.run();
    }
    catch(const std::exception& exception)
    {
        // TODO: Create custom exception with title.
        tessera::TesseraLog::send(tessera::LogType::FATAL, "", exception.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
