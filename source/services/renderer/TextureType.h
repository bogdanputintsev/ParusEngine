#pragma once
#include <array>

namespace parus
{
    enum class TextureType
    {
        ALBEDO,
        NORMAL,
        METALLIC,
        ROUGHNESS,
        AMBIENT_OCCLUSION,
    };

    inline constexpr std::array ALL_TEXTURE_TYPES = {
        TextureType::ALBEDO,
        TextureType::NORMAL,
        TextureType::METALLIC,
        TextureType::ROUGHNESS,
        TextureType::AMBIENT_OCCLUSION
    };

    inline constexpr size_t NUMBER_OF_TEXTURE_TYPES = ALL_TEXTURE_TYPES.size();
}