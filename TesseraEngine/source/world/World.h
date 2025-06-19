#pragma once
#include "camera/SpectatorCamera.h"

namespace tessera
{

    class World final
    {
    public:
        void tick(const float deltaTime);
        
        [[nodiscard]] SpectatorCamera getMainCamera() const { return mainCamera; }
    private:
        SpectatorCamera mainCamera;
    };
        
}
