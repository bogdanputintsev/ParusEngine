#include "MeshFormat.h"

#include <fstream>
#include <sstream>

#include "BinaryStream.h"
#include "FormatHeader.h"
#include "TextureFormat.h"
#include "engine/EngineCore.h"
#include "services/renderer/TextureType.h"
#include "services/renderer/vulkan/material/VulkanMaterial.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"
#include "services/world/Storage.h"
#include "services/Services.h"
#include "services/world/World.h"

namespace parus::serialization
{

    static std::string textureStemForType(
        parus::vulkan::VulkanMaterial& material,
        const parus::TextureType textureType)
    {
        const auto texture = material.getTexture(textureType);
        if (!texture || !texture->sourcePath.has_value())
        {
            return {};
        }

        return std::filesystem::path(*texture->sourcePath).stem().string();
    }

    static void writeMeshPartToStream(std::ostream& stream, const parus::MeshPart& part)
    {
        auto* vulkanMaterial = dynamic_cast<parus::vulkan::VulkanMaterial*>(part.material.get());

        const std::string materialName = vulkanMaterial ? "material" : "";
        const std::string albedoStem   = vulkanMaterial ? textureStemForType(*vulkanMaterial, parus::TextureType::ALBEDO)            : "";
        const std::string normalStem   = vulkanMaterial ? textureStemForType(*vulkanMaterial, parus::TextureType::NORMAL)            : "";
        const std::string metallicStem = vulkanMaterial ? textureStemForType(*vulkanMaterial, parus::TextureType::METALLIC)          : "";
        const std::string roughStem    = vulkanMaterial ? textureStemForType(*vulkanMaterial, parus::TextureType::ROUGHNESS)         : "";
        const std::string aoStem       = vulkanMaterial ? textureStemForType(*vulkanMaterial, parus::TextureType::AMBIENT_OCCLUSION) : "";

        writeString(stream, materialName);
        writeString(stream, albedoStem);
        writeString(stream, normalStem);
        writeString(stream, metallicStem);
        writeString(stream, roughStem);
        writeString(stream, aoStem);

        writeUInt32(stream, static_cast<uint32_t>(part.vertices.size()));
        for (const auto& vertex : part.vertices)
        {
            const math::TrivialVertex trivialVertex = vertex.trivial();
            writeBytes(stream, &trivialVertex, sizeof(trivialVertex));
        }

        writeUInt32(stream, static_cast<uint32_t>(part.indices.size()));
        for (const uint32_t index : part.indices)
        {
            writeUInt32(stream, index);
        }
    }

    std::string writeMesh(
        const parus::Mesh& mesh,
        const std::filesystem::path& outputDir)
    {
        if (!mesh.sourcePath.has_value())
        {
            LOG_WARNING("Skipping mesh with no sourcePath.");
            return {};
        }

        const std::filesystem::path stem = std::filesystem::path(*mesh.sourcePath).stem();
        const std::filesystem::path outputPath = outputDir / (stem.string() + ".pmesh");

        if (std::filesystem::exists(outputPath))
        {
            LOG_INFO("Mesh already exported, skipping: " + stem.string());

            return stem.string();
        }

        std::ostringstream payload(std::ios::binary);

        writeUInt8(payload, static_cast<uint8_t>(mesh.meshType));
        writeUInt32(payload, static_cast<uint32_t>(mesh.meshParts.size()));

        for (const auto& part : mesh.meshParts)
        {
            writeMeshPartToStream(payload, part);
        }

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_WARNING("Cannot open for writing: " + outputPath.string());
            return {};
        }

        const std::string payloadBytes = payload.str();

        FormatHeader header;
        header.magic       = MAGIC_PMESH;
        header.payloadSize = payloadBytes.size();

        writeHeader(file, header);
        file.write(payloadBytes.data(), static_cast<std::streamsize>(payloadBytes.size()));

        if (!file.good())
        {
            LOG_WARNING("File write failed: " + outputPath.string());
            return {};
        }

        return stem.string();
    }

    std::optional<parus::Mesh> readMesh(
        const std::string& stem,
        const std::filesystem::path& meshesDir,
        const std::filesystem::path& texturesDir)
    {
        const std::filesystem::path meshPath = meshesDir / (stem + ".pmesh");

        std::ifstream file(meshPath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_WARNING("Failed to open mesh: " + meshPath.string());

            return std::nullopt;
        }

        FormatHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(FormatHeader));

        if (header.magic != MAGIC_PMESH)
        {
            LOG_WARNING("Invalid magic in mesh file: " + meshPath.string());

            return std::nullopt;
        }

        if (header.version != FORMAT_VERSION)
        {
            LOG_WARNING("Unsupported version in mesh file: " + meshPath.string());

            return std::nullopt;
        }

        const auto meshType = static_cast<MeshType>(readUInt8(file));
        const uint32_t partCount = readUInt32(file);

        const auto storage = Services::get<parus::World>()->getStorage();

        // Load a texture by stem — checks Storage cache first, then reads from .ptex, falls back to default on failure.
        auto loadTextureForMaterial = [&](
            const std::string& textureStem,
            const parus::TextureType textureType,
            parus::vulkan::VulkanMaterial& material)
        {
            if (textureStem.empty())
            {
                return;
            }

            if (storage->hasTexture(textureStem))
            {
                const auto cached = std::dynamic_pointer_cast<parus::vulkan::VulkanTexture2d>(
                    storage->getTexture(textureStem));
                if (cached)
                {
                    material.addOrUpdateTexture(textureType, cached);
                }

                return;
            }

            const auto loaded = readTexture(textureStem, texturesDir);
            if (loaded)
            {
                storage->addNewTexture(textureStem, loaded);
                material.addOrUpdateTexture(textureType, loaded);

                return;
            }

            LOG_WARNING("Texture not found, using default: " + textureStem);
            const auto defaultTexture = std::dynamic_pointer_cast<parus::vulkan::VulkanTexture2d>(
                storage->getDefaultTextureOfType(textureType));
            if (defaultTexture)
            {
                material.addOrUpdateTexture(textureType, defaultTexture);
            }
        };

        parus::Mesh mesh{};
        mesh.meshType   = meshType;
        mesh.sourcePath = stem;

        for (uint32_t partIndex = 0; partIndex < partCount; ++partIndex)
        {
            const std::string materialName  = readString(file);
            const std::string albedoStem    = readString(file);
            const std::string normalStem    = readString(file);
            const std::string metallicStem  = readString(file);
            const std::string roughnessStem = readString(file);
            const std::string aoStem        = readString(file);

            auto material = std::make_shared<parus::vulkan::VulkanMaterial>();

            loadTextureForMaterial(albedoStem,    parus::TextureType::ALBEDO,            *material);
            loadTextureForMaterial(normalStem,    parus::TextureType::NORMAL,            *material);
            loadTextureForMaterial(metallicStem,  parus::TextureType::METALLIC,          *material);
            loadTextureForMaterial(roughnessStem, parus::TextureType::ROUGHNESS,         *material);
            loadTextureForMaterial(aoStem,        parus::TextureType::AMBIENT_OCCLUSION, *material);

            const uint32_t vertexCount = readUInt32(file);
            std::vector<math::Vertex> vertices;
            vertices.reserve(vertexCount);

            for (uint32_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
            {
                math::TrivialVertex trivialVertex{};
                readBytes(file, &trivialVertex, sizeof(trivialVertex));
                vertices.push_back(math::Vertex::fromTrivial(trivialVertex));
            }

            const uint32_t indexCount = readUInt32(file);
            std::vector<uint32_t> indices;
            indices.reserve(indexCount);

            for (uint32_t indexEntry = 0; indexEntry < indexCount; ++indexEntry)
            {
                indices.push_back(readUInt32(file));
            }

            MeshPart part{};
            part.material = std::move(material);
            part.vertices = std::move(vertices);
            part.indices  = std::move(indices);
            mesh.meshParts.push_back(std::move(part));
        }

        if (!file.good())
        {
            LOG_WARNING("File read error in mesh: " + meshPath.string());

            return std::nullopt;
        }

        LOG_INFO("Loaded mesh: " + stem);

        return mesh;
    }

}
