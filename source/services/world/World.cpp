#include "World.h"

#include "services/Services.h"
#include "services/console/Console.h"

namespace parus
{
    void World::init()
    {
        consoleReflection = std::make_unique<ConsoleReflection>(Services::get<Console>(), entityManager, &mainCamera);
    }

    void World::tick(const float deltaTime)
    {
        mainCamera.updateTransform(deltaTime);
    }

    void World::setCameraTransform(const math::Vector3& position, float yaw, float pitch)
    {
        mainCamera.setPosition(position);
        mainCamera.setYaw(yaw);
        mainCamera.setPitch(pitch);
        mainCamera.recalculateDirections();
    }

}
