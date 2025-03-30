#pragma once
#include "Storage.h"
#include "camera/SpectatorCamera.h"

namespace tessera
{

    class World final
    {
    public:
        void tick(const float deltaTime);
        
        [[nodiscard]] SpectatorCamera getMainCamera() const { return mainCamera; }
        [[nodiscard]] std::shared_ptr<Storage> getStorage() const { return storage; }
    private:
        SpectatorCamera mainCamera;

        std::shared_ptr<Storage> storage = std::make_shared<Storage>();
    };
        
}
