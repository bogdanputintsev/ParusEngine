#include "TextureFormat.h"

#include <fstream>
#include <vector>

#include "BinaryStream.h"
#include "FormatHeader.h"
#include "engine/EngineCore.h"
#include "services/renderer/vulkan/builder/VulkanTexture2dBuilder.h"
#include "third-party/stb_image.h"

namespace parus::serialization
{

    std::string writeTexture(
        const parus::Texture& texture,
        const std::filesystem::path& outputDir)
    {
        if (!texture.sourcePath.has_value())
        {
            LOG_WARNING("Skipping texture with no sourcePath.");
            return {};
        }

        const std::filesystem::path stem = std::filesystem::path(*texture.sourcePath).stem();
        const std::filesystem::path outputPath = outputDir / (stem.string() + ".ptex");

        if (std::filesystem::exists(outputPath))
        {
            LOG_INFO("Texture already exported, skipping: " + stem.string());

            return stem.string();
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        stbi_uc* pixels = stbi_load(texture.sourcePath->c_str(), &width, &height, &channels, STBI_default);

        if (!pixels)
        {
            LOG_WARNING("Failed to re-read texture pixels from: " + *texture.sourcePath);
            return {};
        }

        const uint64_t pixelDataSize = static_cast<uint64_t>(width) * static_cast<uint64_t>(height) * static_cast<uint64_t>(channels);

        std::ofstream file(outputPath, std::ios::binary);
        if (!file.is_open())
        {
            stbi_image_free(pixels);
            LOG_WARNING("Cannot open for writing: " + outputPath.string());
            return {};
        }

        const uint64_t payloadSize = sizeof(uint32_t) * 2 + sizeof(uint8_t) * 4 + sizeof(uint64_t) + pixelDataSize;

        FormatHeader header;
        header.magic       = MAGIC_PTEX;
        header.payloadSize = payloadSize;

        writeHeader(file, header);

        writeUInt32(file, static_cast<uint32_t>(width));
        writeUInt32(file, static_cast<uint32_t>(height));
        writeUInt8(file,  static_cast<uint8_t>(channels));
        writeUInt8(file,  0); // texture_type: reserved for iteration 2
        writeUInt8(file,  0); // pixel_format: 0 = raw uint8
        writeUInt8(file,  1); // mip_count: 1
        writeUInt64(file, pixelDataSize);
        writeBytes(file, pixels, static_cast<size_t>(pixelDataSize));

        if (!file.good())
        {
            stbi_image_free(pixels);
            LOG_WARNING("File write failed: " + outputPath.string());
            return {};
        }

        stbi_image_free(pixels);

        return stem.string();
    }

    std::shared_ptr<parus::vulkan::VulkanTexture2d> readTexture(
        const std::string& stem,
        const std::filesystem::path& texturesDir)
    {
        const std::filesystem::path texturePath = texturesDir / (stem + ".ptex");

        std::ifstream file(texturePath, std::ios::binary);
        if (!file.is_open())
        {
            LOG_WARNING("Failed to open texture: " + texturePath.string());

            return nullptr;
        }

        FormatHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(FormatHeader));

        if (header.magic != MAGIC_PTEX)
        {
            LOG_WARNING("Invalid magic in texture file: " + texturePath.string());

            return nullptr;
        }

        if (header.version != FORMAT_VERSION)
        {
            LOG_WARNING("Unsupported version in texture file: " + texturePath.string());

            return nullptr;
        }

        const uint32_t width = readUInt32(file);
        const uint32_t height = readUInt32(file);
        const uint8_t channels = readUInt8(file);

        readUInt8(file); // texture_type: reserved
        readUInt8(file); // pixel_format: reserved
        readUInt8(file); // mip_count:    reserved

        const uint64_t pixelDataSize = readUInt64(file);

        std::vector<stbi_uc> pixels(pixelDataSize);
        file.read(reinterpret_cast<char*>(pixels.data()), static_cast<std::streamsize>(pixelDataSize));

        if (!file.good())
        {
            LOG_WARNING("Failed to read pixel data from: " + texturePath.string());

            return nullptr;
        }

        parus::vulkan::VulkanTexture2d gpuTexture = parus::vulkan::VulkanTexture2dBuilder(stem)
            .buildFromPixels(pixels.data(), static_cast<int>(width), static_cast<int>(height), static_cast<int>(channels));

        gpuTexture.sourcePath = stem;

        LOG_INFO("Loaded texture: " + stem);

        return std::make_shared<parus::vulkan::VulkanTexture2d>(std::move(gpuTexture));
    }

}
