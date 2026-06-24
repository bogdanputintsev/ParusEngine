#pragma once
#include <filesystem>
#include <memory>
#include <string>

#include "services/renderer/Texture.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"

namespace parus::serialization
{

    /**
     * Writes a single .ptex file for the given texture.
     * Re-reads pixel data from sourcePath on disk via stb_image.
     * Returns the stem used as the filename (empty on failure).
     */
    std::string writeTexture(
        const parus::Texture& texture,
        const std::filesystem::path& outputDir);

    /** Loads a texture from a .ptex binary file. Returns nullptr on failure. */
    std::shared_ptr<parus::vulkan::VulkanTexture2d> readTexture(
        const std::string& stem,
        const std::filesystem::path& texturesDir);

}
