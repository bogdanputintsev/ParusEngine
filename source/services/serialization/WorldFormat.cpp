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
#include "services/world/WorldMeshInstance.h"

namespace parus::serialization
{

    void writeWorld(
        const parus::World& world,
        const std::string& sceneName,
        const std::filesystem::path& outputPath)
    {
        const auto camera = world.getMainCamera();
        const auto directionalLight = world.getDirectionalLight();
        const auto pointLights = world.getPointLights();
        const auto meshInstances = world.getMeshInstances();

        // Build deduplicated mesh stem table (STATIC_MESH only; sky is in sky_section)
        std::vector<std::string> meshStems;
        std::unordered_map<Mesh*, uint32_t> meshIndexMap;

        for (const auto& instance : meshInstances)
        {
            if (!instance.mesh || instance.mesh->meshType != MeshType::STATIC_MESH)
            {
                continue;
            }

            auto* meshPtr = instance.mesh.get();

            if (meshIndexMap.contains(meshPtr))
            {
                continue;
            }

            const std::string stem = std::filesystem::path(*instance.mesh->sourcePath).stem().string();
            meshIndexMap[meshPtr] = static_cast<uint32_t>(meshStems.size());
            meshStems.push_back(stem);
        }

        // Determine sky mesh stem
        std::string skyMeshStem;
        const auto storage = world.getStorage();

        for (const auto& mesh : storage->getAllMeshesByType(MeshType::SKY))
        {
            if (mesh->sourcePath.has_value())
            {
                skyMeshStem = std::filesystem::path(*mesh->sourcePath).stem().string();
                break;
            }
        }

        std::ostringstream payload(std::ios::binary);

        // camera_section
        writeVector3(payload, camera.getPosition());
        writeFloat(payload, camera.getYaw());
        writeFloat(payload, camera.getPitch());

        // sky_section
        writeString(payload, skyMeshStem);
        writeVector3(payload, world.getSkyHorizonColor());
        writeVector3(payload, world.getSkyZenithColor());
        writeUInt32(payload, 0); // skybox_settings_size: reserved

        // directional_light_section
        writeVector3(payload, directionalLight.color);
        writeVector3(payload, directionalLight.direction);

        // point_lights_section
        writeUInt32(payload, static_cast<uint32_t>(pointLights.size()));
        for (const auto& light : pointLights)
        {
            writeVector3(payload, light.position);
            writeVector3(payload, light.color);
            writeFloat(payload, light.radius);
            writeFloat(payload, light.intensity);
        }

        // mesh_table_section
        writeUInt32(payload, static_cast<uint32_t>(meshStems.size()));
        for (const auto& stem : meshStems)
        {
            writeString(payload, stem);
        }

        // mesh_instances_section (exclude SKY instances — sky is in sky_section)
        std::vector<const WorldMeshInstance*> staticInstances;
        for (const auto& instance : meshInstances)
        {
            if (instance.mesh && instance.mesh->meshType == MeshType::STATIC_MESH)
            {
                staticInstances.push_back(&instance);
            }
        }

        writeUInt32(payload, static_cast<uint32_t>(staticInstances.size()));
        for (const auto* instance : staticInstances)
        {
            const uint32_t meshIndex = meshIndexMap.at(instance->mesh.get());
            writeUInt32(payload, meshIndex);
            writeMatrix4x4(payload, instance->transform);
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
        sceneData.skyMeshStem     = readString(file);
        sceneData.skyHorizonColor = readVector3(file);
        sceneData.skyZenithColor  = readVector3(file);
        readUInt32(file); // skyboxSettingsSize: reserved

        // directional_light_section
        sceneData.directionalLight.color     = readVector3(file);
        sceneData.directionalLight.direction = readVector3(file);

        // point_lights_section
        const uint32_t pointLightCount = readUInt32(file);
        sceneData.pointLights.reserve(pointLightCount);

        for (uint32_t lightIndex = 0; lightIndex < pointLightCount; ++lightIndex)
        {
            PointLight light{};
            light.position  = readVector3(file);
            light.color     = readVector3(file);
            light.radius    = readFloat(file);
            light.intensity = readFloat(file);
            sceneData.pointLights.push_back(light);
        }

        // mesh_table_section
        const uint32_t meshCount = readUInt32(file);
        sceneData.meshStems.reserve(meshCount);

        for (uint32_t meshIndex = 0; meshIndex < meshCount; ++meshIndex)
        {
            sceneData.meshStems.push_back(readString(file));
        }

        // mesh_instances_section
        const uint32_t instanceCount = readUInt32(file);
        sceneData.meshInstances.reserve(instanceCount);

        for (uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
        {
            MeshInstanceEntry entry{};
            entry.meshIndex = readUInt32(file);
            entry.transform = readMatrix4x4(file);
            sceneData.meshInstances.push_back(entry);
        }

        if (!file.good())
        {
            LOG_ERROR("File read error in world file: " + worldPath.string());

            return std::nullopt;
        }

        return sceneData;
    }

}
