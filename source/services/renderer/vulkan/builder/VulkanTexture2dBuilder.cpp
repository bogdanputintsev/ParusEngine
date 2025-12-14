#include "VulkanTexture2dBuilder.h"

#include <filesystem>

#include "VkDeviceMemoryBuilder.h"
#include "VkImageBuilder.h"
#include "VkImageViewBuilder.h"
#include "VkSamplerBuilder.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"

#define STB_IMAGE_IMPLEMENTATION
#include "services/renderer/vulkan/VulkanRenderer.h"
#include "third-party/stb_image.h"

namespace parus::vulkan
{
    VulkanTexture2dBuilder::VulkanTexture2dBuilder(std::string imageName)
        : debugName(std::move(imageName))
    {
    }

    VulkanTexture2d VulkanTexture2dBuilder::build(const VulkanStorage& storage) const
    {
        VulkanTexture2d newVulkanImage;
        
        newVulkanImage.image = VkImageBuilder(debugName + " Image")
           .setWidth(width)
           .setHeight(height)
           .setMipLevels(numberOfMipLevels)
           .setFormat(format)
           .setTiling(tiling)
           .setUsage(usage)
           .setSamples(numberOfSamples)
           .build(storage);

        if (propertyFlags)
        {
            newVulkanImage.imageMemory = VkDeviceMemoryBuilder(debugName + " Image Memory")
                .setImage(newVulkanImage.image)
                .setPropertyFlags(propertyFlags)
                .build(storage);
        }

        newVulkanImage.imageView = VkImageViewBuilder()
            .setImage(newVulkanImage.image)
            .setFormat(format)
            .setAspectMask(aspectMask)
            .setLevelCount(1)
            .build(debugName + " Image View", storage);
        
        return newVulkanImage;
    }

    VulkanTexture2d VulkanTexture2dBuilder::buildFromFile(const std::string& filePath)
    {
    	debugName = debugName = "[" + filePath + "]";
    	
        ASSERT(std::filesystem::exists(filePath),
			"File " + filePath + " must exist.");
    	ASSERT(std::filesystem::is_regular_file(filePath),
			"File " + filePath + " must be character file."); 
    	
		VulkanTexture2d newTexture{};
		LOG_INFO("Importing texture: " + filePath);

		int textureWidth;
		int textureHeight;
		int textureChannels;
		stbi_uc* pixels = stbi_load(filePath.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
		
    	static constexpr uint64_t NUMBER_OF_CHANNELS = 4;
		const VkDeviceSize imageSize = static_cast<uint64_t>(textureWidth) * static_cast<uint64_t>(textureHeight) * NUMBER_OF_CHANNELS;
		newTexture.maxMipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

		ASSERT(pixels, "Failed to load texture image.");

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

    	const auto& vulkanRenderer = Services::get<VulkanRenderer>();
		vulkanRenderer->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory, 0, imageSize, 0, &data);
		memcpy(data, pixels, static_cast<size_t>(imageSize));
		vkUnmapMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory);
		
		stbi_image_free(pixels);

    	newTexture.image = VkImageBuilder(debugName)
			.setWidth(textureWidth)
			.setHeight(textureHeight)
			.setMipLevels(newTexture.maxMipLevels)
			.setFormat(VK_FORMAT_R8G8B8A8_SRGB)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.setSamples(VK_SAMPLE_COUNT_1_BIT)
			.build(vulkanRenderer->storage);
		
    	newTexture.imageMemory = VkDeviceMemoryBuilder(debugName)
    		.setImage(newTexture.image)
    		.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    		.build(vulkanRenderer->storage);
    	
		vulkanRenderer->transitionImageLayout(newTexture.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, newTexture.maxMipLevels);
		vulkanRenderer->copyBufferToImage(stagingBuffer, newTexture.image, static_cast<uint32_t>(textureWidth), static_cast<uint32_t>(textureHeight));

		vkDestroyBuffer(vulkanRenderer->storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory, nullptr);

		vulkanRenderer->generateMipmaps(newTexture, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight);

		newTexture.imageView = VkImageViewBuilder()
			.setImage(newTexture.image)
			.setFormat(VK_FORMAT_R8G8B8A8_SRGB)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.setLevelCount(newTexture.maxMipLevels)
			.build(debugName, vulkanRenderer->storage);
		
    	VkPhysicalDeviceProperties properties{};
    	vkGetPhysicalDeviceProperties(vulkanRenderer->storage.physicalDevice, &properties);

    	newTexture.sampler = VkSamplerBuilder(debugName)
			.setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
			.setMaxLod(static_cast<float>(newTexture.maxMipLevels))
			.build(vulkanRenderer->storage);
    	
    	return newTexture;
    }

    VulkanTexture2d VulkanTexture2dBuilder::buildFromSolidColor(const math::Vector3& color)
    {
    	VulkanTexture2d newTexture{};
    	
    	const auto& vulkanRenderer = Services::get<vulkan::VulkanRenderer>();
    	
		// 1. Create VkImage
		newTexture.image = VkImageBuilder(debugName)
			.setWidth(1)
			.setHeight(1)
			.setFormat(VK_FORMAT_R8G8B8A8_UNORM)
			.setSamples(VK_SAMPLE_COUNT_1_BIT)
			.build(vulkanRenderer->storage);
		
		// 2. Allocate Memory
    	newTexture.imageMemory = VkDeviceMemoryBuilder(debugName)
    		.setImage(newTexture.image)
    		.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
    		.build(vulkanRenderer->storage);

		// 3. Transition Image Layout
		vulkanRenderer->transitionImageLayout(
			newTexture.image,
			VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1);

		// 4. Upload Solid Color
		const uint8_t colorData[4] = {
			static_cast<uint8_t>(color.x * 255.0f),
			static_cast<uint8_t>(color.y * 255.0f),
			static_cast<uint8_t>(color.z * 255.0f),
			255
		};
    	
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		vulkanRenderer->createBuffer(sizeof(colorData), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
			stagingBuffer, stagingBufferMemory);

		utils::setDebugName(vulkanRenderer->storage, stagingBuffer, VK_OBJECT_TYPE_BUFFER, debugName.c_str());
		utils::setDebugName(vulkanRenderer->storage, stagingBufferMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, debugName.c_str());
		
		void* data;
		vkMapMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory, 0, sizeof(colorData), 0, &data);
		memcpy(data, colorData, sizeof(colorData));
		vkUnmapMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory);

		vulkanRenderer->copyBufferToImage(stagingBuffer, newTexture.image, 1, 1);

		vkDestroyBuffer(vulkanRenderer->storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(vulkanRenderer->storage.logicalDevice, stagingBufferMemory, nullptr);

		// 5. Transition to Shader-Readable Layout
		vulkanRenderer->transitionImageLayout(
			newTexture.image,
			VK_FORMAT_R8G8B8A8_UNORM, 
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			1);

		// 6. Create Image View
		newTexture.imageView = VkImageViewBuilder()
			.setImage(newTexture.image)
			.setFormat(VK_FORMAT_R8G8B8A8_UNORM)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.setLevelCount(1)
			.build(debugName, vulkanRenderer->storage);
		
		// 7. Create Sampler
		newTexture.sampler = VkSamplerBuilder(debugName)
    		.build(vulkanRenderer->storage);

		return newTexture;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setImageSize(const uint32_t newWidth, const uint32_t newHeight)
    {
        width = newWidth;
        height = newHeight;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setWidth(const uint32_t newWidth)
    {
        width = newWidth;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setHeight(const uint32_t newHeight)
    {
        height = newHeight;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setNumberOfMipLevels(const uint32_t newNumberOfMipLevels)
    {
        numberOfMipLevels = newNumberOfMipLevels;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setNumberOfSamples(const VkSampleCountFlagBits newNumberOfSamples)
    {
        numberOfSamples = newNumberOfSamples;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setFormat(const VkFormat newFormat)
    {
        format = newFormat;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setAspectMask(const VkImageAspectFlagBits newAspectMask)
    {
        aspectMask = newAspectMask;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setTiling(const VkImageTiling newTiling)
    {
        tiling = newTiling;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setUsage(const VkImageUsageFlags newUsage)
    {
        usage = newUsage;
        return *this;
    }

    VulkanTexture2dBuilder& VulkanTexture2dBuilder::setPropertyFlags(const VkMemoryPropertyFlags newPropertyFlags)
    {
        propertyFlags = newPropertyFlags;
        return *this;
    }
}
    
