#include "Serialization.h"

#include <algorithm>
#include <filesystem>

#include "MeshFormat.h"
#include "SceneData.h"
#include "TextureFormat.h"
#include "WorldFormat.h"
#include "engine/EngineCore.h"
#include "engine/utils/Utils.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/console/Console.h"
#include "services/platform/Platform.h"
#include "services/renderer/vulkan/VulkanRenderer.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"
#include "services/world/entity/Components.h"
#include "services/world/entity/EntityManager.h"

namespace parus
{

    static constexpr const char* SCENES_DIR   = "bin/assets/scenes";
    static constexpr const char* MESHES_DIR   = "bin/assets/meshes";
    static constexpr const char* TEXTURES_DIR = "bin/assets/textures";

    static void updateWindowTitle(const std::string& sceneName)
    {
        const std::string baseTitle = Services::get<Configs>()->getOrDefault<std::string>("Window", "title", "");
        if (baseTitle.empty())
        {
            return;
        }
        
        const std::string title = baseTitle + " (" + sceneName + ")";
        Services::get<Platform>()->setWindowTitle(title);
    }

    void Serialization::registerConsoleCommands()
    {
        const auto console = Services::get<Console>();

        console->registerConsoleCommand("save", [](const std::vector<std::string>& args) -> std::string
        {
            if (args.empty())
            {
                const std::string_view currentName = Services::get<World>()->getCurrentSceneName();
                if (currentName.empty())
                {
                    return "No scene is currently open. Usage: save <scene_name>";
                }

                return Services::get<Serialization>()->saveCurrentWorld(std::string(currentName));
            }

            return Services::get<Serialization>()->saveCurrentWorld(args[0]);
        });

        console->registerConsoleCommand("open", [](const std::vector<std::string>& args) -> std::string
        {
            if (args.empty())
            {
                return "Usage: open <scene_name>";
            }

            return Services::get<Serialization>()->importWorld(args[0]);
        });

        console->registerConsoleCommand("import", [](const std::vector<std::string>& args) -> std::string
        {
            if (args.empty())
            {
                return "Usage: import <relative_path>  (relative to bin/, e.g. terrain/floor.obj)";
            }

            std::string relativePath;
            for (size_t i = 0; i < args.size(); ++i)
            {
                if (i > 0)
                {
                    relativePath += ' ';
                }
                relativePath += args[i];
            }

            static constexpr const char* ASSETS_BIN_DIR = "bin/";
            const std::string filePath = ASSETS_BIN_DIR + relativePath;

            if (!std::filesystem::exists(filePath))
            {
                return "File not found: " + filePath;
            }

            const std::string extension = std::filesystem::path(filePath).extension().string();
            if (!utils::string::equalsIgnoreCase(extension, ".obj"))
            {
                return "Unsupported format: " + extension + ". Only .obj files are supported.";
            }

            const auto renderer = Services::get<Renderer>();
            auto* vulkanRenderer = dynamic_cast<parus::vulkan::VulkanRenderer*>(renderer.get());
            ASSERT(vulkanRenderer, "VulkanRenderer type is expected.");

            RUN_ASYNC(vulkanRenderer->importMesh(filePath););

            return "Importing: " + filePath;
        });
    }

    std::string Serialization::saveCurrentWorld(const std::string& sceneName)
    {
        const std::filesystem::path scenesDir   = SCENES_DIR;
        const std::filesystem::path meshesDir   = MESHES_DIR;
        const std::filesystem::path texturesDir = TEXTURES_DIR;

        std::filesystem::create_directories(scenesDir);
        std::filesystem::create_directories(meshesDir);
        std::filesystem::create_directories(texturesDir);

        const auto world   = Services::get<World>();
        const auto storage = world->getStorage();
        const auto pool    = Services::get<ThreadPool>();

        for (const auto& mesh : storage->getAllMeshes())
        {
            if (!mesh)
            {
                continue;
            }

            RUN_ASYNC(serialization::writeMesh(*mesh, meshesDir););
        }

        for (const auto& texture : storage->getAllTextures())
        {
            if (!texture || !texture->sourcePath.has_value())
            {
                continue;
            }

            RUN_ASYNC(serialization::writeTexture(*texture, texturesDir););
        }

        pool->waitUntilDone();

        const std::filesystem::path outputPath = scenesDir / (sceneName + ".pworld");
        serialization::writeWorld(*world, sceneName, outputPath);

        const auto allMeshes = storage->getAllMeshes();
        const uint32_t meshCount = static_cast<uint32_t>(
            std::count_if(allMeshes.begin(), allMeshes.end(), [](const auto& mesh)
            {
                return mesh && mesh->sourcePath.has_value();
            }));
        const uint32_t textureCount  = static_cast<uint32_t>(storage->getAllTextures().size());
        const uint32_t instanceCount = static_cast<uint32_t>(world->getEntityManager()->getMeshEntities().size());

        world->setCurrentSceneName(sceneName);
        updateWindowTitle(sceneName);

        const std::string result = "Scene saved: " + sceneName
            + "\n\tMeshes:    " + std::to_string(meshCount)
            + "\n\tTextures:  " + std::to_string(textureCount)
            + "\n\tInstances: " + std::to_string(instanceCount);
        LOG_INFO(result);

        return result;
    }

    std::string Serialization::importWorld(const std::string& sceneName)
    {
        const std::filesystem::path scenesDir = SCENES_DIR;

        std::optional<serialization::SceneData> sceneData = serialization::readWorld(sceneName, scenesDir);

        if (!sceneData)
        {
            return "Scene not found: " + sceneName;
        }

        const auto world   = Services::get<World>();
        const auto storage = world->getStorage();
        const auto pool    = Services::get<ThreadPool>();

        const auto renderer  = Services::get<Renderer>();
        auto* vulkanRenderer = dynamic_cast<parus::vulkan::VulkanRenderer*>(renderer.get());
        ASSERT(vulkanRenderer, "VulkanRenderer type is expected.");

        world->getEntityManager()->clearSceneEntities();
        vulkanRenderer->deviceWaitIdle();
        vulkanRenderer->cleanupSceneTextures();
        storage->clearSceneAssets();

        for (const std::string& meshStem : sceneData->meshStems)
        {
            RUN_ASYNC(
                std::optional<Mesh> loadedMesh = serialization::readMesh(meshStem, MESHES_DIR, TEXTURES_DIR);
                if (loadedMesh)
                {
                    storage->addNewMesh(meshStem, std::make_shared<Mesh>(std::move(*loadedMesh)));
                }
            );
        }

        pool->waitUntilDone();

        world->setCameraTransform(sceneData->cameraPosition, sceneData->cameraYaw, sceneData->cameraPitch);

        const auto entityManager = world->getEntityManager();

        const EntityId sunId = entityManager->spawn("Sun");
        entityManager->addDirectionalLightComponent(sunId, DirectionalLightComponent{
            .color     = sceneData->directionalLight.color,
            .direction = sceneData->directionalLight.direction
        });

        // The built-in sky mesh has no sourcePath, so its stem is always empty; it survives
        // Storage::clearSceneAssets() (which preserves MeshType::SKY meshes), so it is looked up
        // by type rather than by the (empty) stem written for it.
        const auto skyMeshes = storage->getAllMeshesByType(MeshType::SKY);
        if (!skyMeshes.empty())
        {
            const EntityId skyId = entityManager->spawn("Skybox");
            entityManager->addSkyboxComponent(skyId, SkyboxComponent{
                .mesh         = skyMeshes.front(),
                .horizonColor = sceneData->skybox.horizonColor,
                .zenithColor  = sceneData->skybox.zenithColor
            });
        }

        for (const serialization::EntityEntry& entry : sceneData->entities)
        {
            const EntityId entityId = entityManager->spawn(entry.name);
            entityManager->setMobility(entityId, entry.mobility);
            entityManager->setTransform(entityId, entry.transform);

            if (entry.meshComponent)
            {
                if (entry.meshComponent->meshIndex >= sceneData->meshStems.size())
                {
                    LOG_WARNING("Skipping mesh component with out-of-range mesh index: " + std::to_string(entry.meshComponent->meshIndex));
                }
                else
                {
                    const std::string& meshStem = sceneData->meshStems[entry.meshComponent->meshIndex];
                    const std::shared_ptr<Mesh> mesh = storage->getMeshByPath(meshStem);

                    if (mesh)
                    {
                        entityManager->addMeshComponent(entityId, MeshComponent{ mesh });
                    }
                    else
                    {
                        LOG_WARNING("Skipping mesh component - mesh not loaded: " + meshStem);
                    }
                }
            }

            if (entry.pointLightComponent)
            {
                entityManager->addPointLightComponent(entityId, PointLightComponent{
                    .color     = entry.pointLightComponent->color,
                    .radius    = entry.pointLightComponent->radius,
                    .intensity = entry.pointLightComponent->intensity
                });
            }
        }

        vulkanRenderer->applySceneFromWorld();

        world->setCurrentSceneName(sceneName);
        updateWindowTitle(sceneName);

        const uint32_t loadedMeshCount     = static_cast<uint32_t>(sceneData->meshStems.size());
        const uint32_t loadedInstanceCount = static_cast<uint32_t>(entityManager->getMeshEntities().size());
        const uint32_t pointLightCount     = static_cast<uint32_t>(entityManager->getPointLightEntities().size());

        const math::Vector3& camPos = sceneData->cameraPosition;

        const std::string result = "Scene loaded: " + sceneName
            + "\n\tMeshes:       " + std::to_string(loadedMeshCount)
            + "\n\tInstances:    " + std::to_string(loadedInstanceCount)
            + "\n\tPoint lights: " + std::to_string(pointLightCount)
            + "\n\tCamera pos:   " + std::to_string(camPos.x) + ", " + std::to_string(camPos.y) + ", " + std::to_string(camPos.z)
            + "\n\tCamera yaw:   " + std::to_string(sceneData->cameraYaw)
            + "\n\tCamera pitch: " + std::to_string(sceneData->cameraPitch);
        LOG_INFO(result);

        return result;
    }

}
