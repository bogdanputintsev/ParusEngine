#include "VulkanMaterial.h"

#include "engine/EngineCore.h"
#include "services/Services.h"
#include "services/renderer/TextureType.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"
#include "services/world/World.h"

namespace parus::vulkan
{
    VulkanMaterial::VulkanMaterial()
    {
        iterateAllTextureTypes([&](const parus::TextureType textureType)
        {
            const auto defaultTexture = std::dynamic_pointer_cast<VulkanTexture2d>(
                Services::get<World>()->getStorage()->getDefaultTextureOfType(textureType));
            ASSERT(defaultTexture, "Expected VulkanTexture2d as default texture.");
            textures[textureType] = defaultTexture;
        });
    }

    void VulkanMaterial::addOrUpdateTexture(const parus::TextureType textureType, const std::shared_ptr<VulkanTexture2d>& newTexture)
    {
        if (!newTexture)
        {
            const auto defaultTexture = std::dynamic_pointer_cast<VulkanTexture2d>(
                Services::get<World>()->getStorage()->getDefaultTextureOfType(textureType));
            ASSERT(defaultTexture, "Expected VulkanTexture2d as default texture.");
            textures.insert_or_assign(textureType, defaultTexture);
            return;
        }
        
        textures.insert_or_assign(textureType, newTexture);
    }

    std::shared_ptr<VulkanTexture2d> VulkanMaterial::getTexture(const parus::TextureType textureType)
    {
        DEBUG_ASSERT(textures.contains(textureType), "Texture of each type must always exist.");
        return textures.at(textureType);
    }

    std::vector<std::shared_ptr<const VulkanTexture2d>> VulkanMaterial::getAllTextures() const
    {
        std::vector<std::shared_ptr<const VulkanTexture2d>> allTextures;
        allTextures.reserve(parus::NUMBER_OF_TEXTURE_TYPES);

        for (parus::TextureType textureType : parus::ALL_TEXTURE_TYPES)
        {
            DEBUG_ASSERT(textures.contains(textureType), "Material must always contain all types of textures.");
            allTextures.push_back(textures.at(textureType));
        }

        return allTextures;
    }

    void VulkanMaterial::iterateAllTextures(const std::function<void(parus::TextureType, const std::shared_ptr<const VulkanTexture2d>&)>& callback) const
    {
        const auto allTextures = getAllTextures();

        ASSERT(allTextures.size() == parus::NUMBER_OF_TEXTURE_TYPES, "Some texture types are missing in material.");

        for (size_t i = 0; i < parus::NUMBER_OF_TEXTURE_TYPES; i++)
        {
            callback(parus::ALL_TEXTURE_TYPES[i], allTextures[i]);
        }
    }

    void VulkanMaterial::iterateAllTextureTypes(const std::function<void(parus::TextureType)>& callback)
    {
        for (const parus::TextureType type : parus::ALL_TEXTURE_TYPES)
        {
            callback(type);
        }
    }

    void VulkanMaterial::iterateAllTextureTypes(const std::function<void(int, parus::TextureType)>& callback)
    {
        for (int i = 0; i < static_cast<int>(parus::NUMBER_OF_TEXTURE_TYPES); i++)
        {
            callback(i, parus::ALL_TEXTURE_TYPES[i]);
        }
    }
}
