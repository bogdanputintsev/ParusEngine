#pragma once
#include <functional>
#include <memory>
#include <unordered_map>

#include "services/renderer/Material.h"
#include "services/renderer/TextureType.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"

namespace parus::vulkan
{

    class VulkanMaterial final : public parus::Material
    {
    public:
        VulkanMaterial();

        void addOrUpdateTexture(const parus::TextureType textureType, const std::shared_ptr<VulkanTexture2d>& newTexture);
        std::shared_ptr<VulkanTexture2d> getTexture(const parus::TextureType textureType);

        [[nodiscard]] std::vector<std::shared_ptr<const VulkanTexture2d>> getAllTextures() const;
        void iterateAllTextures(const std::function<void(parus::TextureType, const std::shared_ptr<const VulkanTexture2d>&)>& callback) const;
        static void iterateAllTextureTypes(const std::function<void(parus::TextureType)>& callback);
        static void iterateAllTextureTypes(const std::function<void(int, parus::TextureType)>& callback);

        VkDescriptorSet materialDescriptorSet = VK_NULL_HANDLE;
    private:
        std::unordered_map<parus::TextureType, std::shared_ptr<VulkanTexture2d>> textures;
    };

}

