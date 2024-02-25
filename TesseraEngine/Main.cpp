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
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
