#include "World.h"

#include "services/Services.h"
#include "services/console/Console.h"

namespace parus
{
    void World::init()
    {
        registerConsoleCommands();
    }

    void World::tick(const float deltaTime)
    {
        mainCamera.updateTransform(deltaTime);
    }

    void World::registerConsoleCommands()
    {
        auto console = Services::get<Console>();

        console->registerConsoleCommand("get camera position", [this]()
        {
            const auto cameraPosition = mainCamera.getPosition();
            return std::string("Camera position:")
                + "\n\tX: " + std::to_string(cameraPosition.x)
                + "\n\tY: " + std::to_string(cameraPosition.y)
                + "\n\tZ: " + std::to_string(cameraPosition.z);
        });
    }

}
