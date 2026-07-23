#include "WorldFormat.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <filesystem>

#include "BinaryStream.h"
#include "FormatHeader.h"
#include "SceneData.h"
#include "engine/EngineCore.h"
#include "services/renderer/vulkan/mesh/Mesh.h"
#include "services/world/entity/EntityManager.h"

namespace parus::serialization
{

    void writeWorld(
        const parus::World& world,
        const std::string& sceneName,
        const std::filesystem::path& outputPath)
    {
        const auto camera = world.getMainCamera();
        const auto entityManager = world.getEntityManager();
        const auto allEntities = entityManager->getAllEntities();

        // Build deduplicated mesh stem table (GEOMETRY only; sky is written separately below).
        std::vector<std::string> meshStems;
        std::unordered_map<Mesh*, uint32_t> meshIndexMap;

        for (const auto* entity : allEntities)
        {
            const auto* meshComponent = entityManager->getMeshComponent(entity->id);
            if (!meshComponent || !meshComponent->mesh || meshComponent->mesh->meshType != MeshType::GEOMETRY)
            {
                continue;
            }

            auto* meshPtr = meshComponent->mesh.get();

            if (meshIndexMap.contains(meshPtr))
            {
                continue;
            }

            const std::string stem = std::filesystem::path(*meshComponent->mesh->sourcePath).stem().string();
            meshIndexMap[meshPtr] = static_cast<uint32_t>(meshStems.size());
            meshStems.push_back(stem);
        }

        // Determine sky mesh stem from the skybox entity.
        std::string skyMeshStem;
        if (const auto* skyboxComponent = entityManager->getSkyboxComponent();
            skyboxComponent && skyboxComponent->mesh && skyboxComponent->mesh->sourcePath.has_value())
        {
            skyMeshStem = std::filesystem::path(*skyboxComponent->mesh->sourcePath).stem().string();
        }

        std::ostringstream payload(std::ios::binary);

        // camera_section
        writeVector3(payload, camera.getPosition());
        writeFloat(payload, camera.getYaw());
        writeFloat(payload, camera.getPitch());

        // sky_section
        const auto* skyboxComponent = entityManager->getSkyboxComponent();
        writeString(payload, skyMeshStem);
        writeVector3(payload, skyboxComponent ? skyboxComponent->horizonColor : math::Vector3());
        writeVector3(payload, skyboxComponent ? skyboxComponent->zenithColor : math::Vector3());
        writeUInt32(payload, 0); // skybox_settings_size: reserved

        // directional_light_section
        const auto* directionalLightComponent = entityManager->getDirectionalLightComponent();
        writeVector3(payload, directionalLightComponent ? directionalLightComponent->color : math::Vector3());
        writeVector3(payload, directionalLightComponent ? directionalLightComponent->direction : math::Vector3());

        // mesh_table_section
        writeUInt32(payload, static_cast<uint32_t>(meshStems.size()));
        for (const auto& stem : meshStems)
        {
            writeString(payload, stem);
        }

        // entity_table_section (excludes the directional-light entity and the skybox entity - those are
        // written directly in their own sections above, not as generic rows)
        std::vector<const Entity*> exportedEntities;
        for (const auto* entity : allEntities)
        {
            if (entityManager->getDirectionalLightEntity() == entity || entityManager->getSkyboxEntity() == entity)
            {
                continue;
            }

            exportedEntities.push_back(entity);
        }

        writeUInt32(payload, static_cast<uint32_t>(exportedEntities.size()));
        for (const auto* entity : exportedEntities)
        {
            writeString(payload, entity->name);
            writeUInt8(payload, static_cast<uint8_t>(entity->mobility));
            writeMatrix4x4(payload, entity->transform);

            const auto* meshComponent = entityManager->getMeshComponent(entity->id);
            const bool hasMesh = meshComponent && meshComponent->mesh && meshComponent->mesh->meshType == MeshType::GEOMETRY;
            writeUInt8(payload, hasMesh ? 1 : 0);
            if (hasMesh)
            {
                writeUInt32(payload, meshIndexMap.at(meshComponent->mesh.get()));
            }

            const auto* pointLightComponent = entityManager->getPointLightComponent(entity->id);
            writeUInt8(payload, pointLightComponent ? 1 : 0);
            if (pointLightComponent)
            {
                writeVector3(payload, pointLightComponent->color);
                writeFloat(payload, pointLightComponent->radius);
                writeFloat(payload, pointLightComponent->intensity);
            }
        }

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Cannot open for writing: " + outputPath.string());

            return;
        }

        const std::string payloadBytes = payload.str();

        FormatHeader header;
        header.magic       = MAGIC_PWORLD;
        header.payloadSize = payloadBytes.size();

        writeHeader(file, header);
        file.write(payloadBytes.data(), static_cast<std::streamsize>(payloadBytes.size()));

        if (!file.good())
        {
            LOG_ERROR("File write failed: " + outputPath.string());

            return;
        }

        LOG_INFO("Exported world: " + sceneName);
    }

    std::optional<SceneData> readWorld(
        const std::string& sceneName,
        const std::filesystem::path& scenesDir)
    {
        const std::filesystem::path worldPath = scenesDir / (sceneName + ".pworld");

        std::ifstream file(worldPath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_ERROR("Failed to open world file: " + worldPath.string());

            return std::nullopt;
        }

        FormatHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(FormatHeader));

        if (header.magic != MAGIC_PWORLD)
        {
            LOG_ERROR("Invalid magic in world file: " + worldPath.string());

            return std::nullopt;
        }

        if (header.version != FORMAT_VERSION)
        {
            LOG_ERROR("Unsupported version in world file: " + worldPath.string());

            return std::nullopt;
        }

        SceneData sceneData{};

        // camera_section
        sceneData.cameraPosition = readVector3(file);
        sceneData.cameraYaw      = readFloat(file);
        sceneData.cameraPitch    = readFloat(file);

        // sky_section
        sceneData.skybox.meshStem     = readString(file);
        sceneData.skybox.horizonColor = readVector3(file);
        sceneData.skybox.zenithColor  = readVector3(file);
        readUInt32(file); // skyboxSettingsSize: reserved

        // directional_light_section
        sceneData.directionalLight.color     = readVector3(file);
        sceneData.directionalLight.direction = readVector3(file);

        // mesh_table_section
        const uint32_t meshCount = readUInt32(file);
        sceneData.meshStems.reserve(meshCount);

        for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
        {
            sceneData.meshStems.push_back(readString(file));
        }

        // entity_table_section
        const uint32_t entityCount = readUInt32(file);
        sceneData.entities.reserve(entityCount);

        for (uint32_t entityIndex = 0; entityIndex < entityCount; ++entityIndex)
        {
            EntityEntry entry{};
            entry.name      = readString(file);
            entry.mobility  = static_cast<parus::Mobility>(readUInt8(file));
            entry.transform = readMatrix4x4(file);

            const bool hasMesh = readUInt8(file) != 0;
            if (hasMesh)
            {
                EntityMeshEntry meshEntry{};
                meshEntry.meshIndex = readUInt32(file);
                entry.meshComponent = meshEntry;
            }

            const bool hasPointLight = readUInt8(file) != 0;
            if (hasPointLight)
            {
                EntityPointLightEntry pointLightEntry{};
                pointLightEntry.color     = readVector3(file);
                pointLightEntry.radius    = readFloat(file);
                pointLightEntry.intensity = readFloat(file);
                entry.pointLightComponent = pointLightEntry;
            }

            sceneData.entities.push_back(entry);
        }

        if (!file.good())
        {
            LOG_ERROR("File read error in world file: " + worldPath.string());

            return std::nullopt;
        }

        return sceneData;
    }

}
