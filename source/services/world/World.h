#pragma once
#include <memory>
#include <string>
#include <string_view>

#include "Storage.h"
#include "camera/SpectatorCamera.h"
#include "entity/EntityManager.h"
#include "services/Service.h"

namespace parus
{

    /** Owns the current scene: camera, asset storage, and every entity/component in the world. */
    class World final : public Service
    {
    public:
        void init();
        void tick(const float deltaTime);
        void registerConsoleCommands();

        [[nodiscard]] SpectatorCamera getMainCamera() const { return mainCamera; }

        /** Sets the camera position, yaw and pitch, then recalculates direction vectors. */
        void setCameraTransform(const math::Vector3& position, float yaw, float pitch);

        [[nodiscard]] std::shared_ptr<Storage> getStorage() const { return storage; }
        [[nodiscard]] std::shared_ptr<EntityManager> getEntityManager() const { return entityManager; }

        [[nodiscard]] std::string_view getCurrentSceneName() const { return currentSceneName; }
        void setCurrentSceneName(const std::string& name) { currentSceneName = name; }

    private:
        SpectatorCamera mainCamera;
        std::shared_ptr<Storage> storage = std::make_shared<Storage>();
        std::shared_ptr<EntityManager> entityManager = std::make_shared<EntityManager>();
        std::string currentSceneName;
    };

}
