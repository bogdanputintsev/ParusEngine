#include "World.h"

namespace tessera
{
    
    void World::tick(const float deltaTime)
    {
        mainCamera.updateTransform(deltaTime);
    }
    
}
