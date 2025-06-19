#include "TextureImporter.h"

#include <stb_image.h>

#include "Core.h"

namespace tessera
{
    
    Texture TextureImporter::importFromFile(const std::string& filePath)
    {
    	ASSERT(std::filesystem::exists(filePath),
			"File " + filePath + " must exist.");
    	ASSERT(std::filesystem::is_regular_file(filePath),
			"File " + filePath + " must be character file."); 
    	
		Texture newTexture{};
		LOG_INFO("Importing texture: " + filePath);

		int textureWidth;
		int textureHeight;
		int textureChannels;
		stbi_uc* pixels = stbi_load(filePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		
		const VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth) * static_cast<uint64_t>(textureHeight) * 4;
		newTexture.maxMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

		ASSERT(pixels, "failed to load texture image.");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

    	const auto& vulkanRenderer = CORE->renderer;
		vulkanRenderer->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkanRenderer->logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vulkanRenderer->logicalDevice, stagingBufferMemory);
		
		stbi_image_free(pixels);

		vulkanRenderer->createImage(textureWidth, textureHeight,
			newTexture.maxMipLevels,
			VK_SAMPLE_COUNT_1_BIT,
			VK_FORMAT_R8G8B8A8_SRGB,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			newTexture.textureImage,
			newTexture.textureImageMemory);
		
		vulkanRenderer->transitionImageLayout(newTexture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newTexture.maxMipLevels);
		vulkanRenderer->copyBufferToImage(stagingBuffer, newTexture.textureImage, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

		vkDestroyBuffer(vulkanRenderer->logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(vulkanRenderer->logicalDevice, stagingBufferMemory, nullptr);

		vulkanRenderer->generateMipmaps(newTexture, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight);

		newTexture.textureImageView = vulkanRenderer->createImageView(newTexture.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, newTexture.maxMipLevels);
		newTexture.textureSampler = vulkanRenderer->createTextureSampler(newTexture.maxMipLevels);

        return newTexture;
    }
}
