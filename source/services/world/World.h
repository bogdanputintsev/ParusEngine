#pragma once
#include <span>
#include <vector>

#include "Storage.h"
#include "SceneLight.h"
#include "WorldMeshInstance.h"
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

        /** Sets the camera position, yaw and pitch, then recalculates direction vectors. */
        void setCameraTransform(const math::Vector3& position, float yaw, float pitch)
        {
            mainCamera.setPosition(position);
            mainCamera.setYaw(yaw);
            mainCamera.setPitch(pitch);
            mainCamera.recalculateDirections();
        }
        [[nodiscard]] std::shared_ptr<Storage> getStorage() const { return storage; }

        // --- Scene state (source of truth for serialization) ---

        [[nodiscard]] DirectionalLight getDirectionalLight() const { return directionalLight; }
        void setDirectionalLight(const DirectionalLight& light) { directionalLight = light; }

        [[nodiscard]] std::span<const PointLight> getPointLights() const { return pointLights; }
        void addPointLight(const PointLight& light) { pointLights.push_back(light); }
        void clearPointLights() { pointLights.clear(); }

        [[nodiscard]] math::Vector3 getSkyHorizonColor() const { return skyHorizonColor; }
        [[nodiscard]] math::Vector3 getSkyZenithColor() const { return skyZenithColor; }
        void setSkyColors(const math::Vector3& horizon, const math::Vector3& zenith)
        {
            skyHorizonColor = horizon;
            skyZenithColor  = zenith;
        }

        [[nodiscard]] std::span<const WorldMeshInstance> getMeshInstances() const { return meshInstances; }
        void addMeshInstance(const WorldMeshInstance& instance) { meshInstances.push_back(instance); }
        void clearMeshInstances() { meshInstances.clear(); }

        [[nodiscard]] std::string_view getCurrentSceneName() const { return currentSceneName; }
        void setCurrentSceneName(const std::string& name) { currentSceneName = name; }

    private:
        SpectatorCamera mainCamera;
        std::shared_ptr<Storage> storage = std::make_shared<Storage>();

        DirectionalLight directionalLight;
        std::vector<PointLight> pointLights;
        math::Vector3 skyHorizonColor;
        math::Vector3 skyZenithColor;
        std::vector<WorldMeshInstance> meshInstances;
        std::string currentSceneName;
    };

}
