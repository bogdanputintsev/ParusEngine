#pragma once
#include "Storage.h"
#include "camera/SpectatorCamera.h"
#include "services/Service.h"

namespace parus
{

    class World final : public Service
    {
    public:
        void init();
        void tick(const float deltaTime);
        void registerConsoleCommands();

        [[nodiscard]] SpectatorCamera getMainCamera() const { return mainCamera; }
        [[nodiscard]] std::shared_ptr<Storage> getStorage() const { return storage; }
    private:
        SpectatorCamera mainCamera;

        std::shared_ptr<Storage> storage = std::make_shared<Storage>();
    };
        
}
