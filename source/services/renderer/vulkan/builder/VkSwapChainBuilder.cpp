#include "VkSwapChainBuilder.h"

#include "services/config/Configs.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"


namespace parus::vulkan
{
    
    void VkSwapChainBuilder::build(VulkanStorage& storage)
    {
		const auto [capabilities, formats, presentModes] = utils::querySwapChainSupport(storage.physicalDevice, storage.surface);

		const auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
		const VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
		const VkExtent2D extent = chooseSwapExtent(capabilities);

		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		ASSERT(extent.width != 0 && extent.height != 0, "Swap chain extent is invalid (window may be minimized)");

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = storage.surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format;
		createInfo.imageColorSpace = colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const auto [graphicsFamily, presentFamily] = utils::findQueueFamilies(storage.physicalDevice, storage.surface);
		ASSERT(graphicsFamily.has_value() && presentFamily.has_value(), "Queue families are not complete.");
		const uint32_t queueFamilyIndices[] = { graphicsFamily.value(), presentFamily.value() };

		if (graphicsFamily != presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		ASSERT(vkCreateSwapchainKHR(storage.logicalDevice, &createInfo, nullptr, &storage.swapChain) == VK_SUCCESS, "Failed to create swap chain.");

		std::vector<VkImage> swapChainImages;
		vkGetSwapchainImagesKHR(storage.logicalDevice, storage.swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(storage.logicalDevice, storage.swapChain, &imageCount, swapChainImages.data());

		storage.swapChainDetails = {
			.swapChainImageFormat = format,
			.swapChainExtent = extent,
			.swapChainImages = swapChainImages
		};
    }

    VkSurfaceFormatKHR VkSwapChainBuilder::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
    {
    	ASSERT(!availableFormats.empty(), "No swap chain formats available.");

    	for (const auto& availableFormat : availableFormats)
    	{
    		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
    		{
    			return availableFormat;
    		}
    	}

    	return availableFormats[0];
    }

    VkPresentModeKHR VkSwapChainBuilder::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
    {
    	for (const auto& availablePresentMode : availablePresentModes)
    	{
    		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
    		{
    			return availablePresentMode;
    		}
    	}

    	// Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
    	return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VkSwapChainBuilder::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
    {
    	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
    	{
    		return capabilities.currentExtent;
    	}

    	const int width = Services::get<Configs>()->getAsBool("Window", "width").value_or(0);
    	const int height = Services::get<Configs>()->getAsBool("Window", "width").value_or(0);

    	ASSERT(width != 0 && height != 0, "Failed to get window width and height.");
		
    	VkExtent2D actualExtent = {
    		static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

    	actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    	actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    	return actualExtent;
    }
}
