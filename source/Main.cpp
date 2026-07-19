#include "engine/EngineCore.h"
#include "engine/application/Application.h"


/**
 * Entry point. Runs engine lifecycle: init, main loop, clean shutdown.
 * Any uncaught exception is logged as fatal and aborts the run.
 */
int main(const int argc, const char* argv[])
{
    parus::Application application;

    try
    {
        application.init(argc, argv);
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
