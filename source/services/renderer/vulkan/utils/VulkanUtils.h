#pragma once

#include <vulkan/vulkan_core.h>

#include "services/renderer/vulkan/VulkanRenderer.h"
#include "engine/EngineCore.h"

namespace parus::vulkan::utils
{

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};
	
    QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};

		[[nodiscard]] bool isComplete() const { return !formats.empty() && !presentModes.empty(); }
	};
	
	SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
    
	constexpr bool validationLayersEnabled()
	{
#ifdef IN_DEBUG_MODE
		return true;
#else
		return false;
#endif
	}
	
    static constexpr std::array<const char*, 1> REQUIRED_VALIDATION_LAYERS = { "VK_LAYER_KHRONOS_validation" };
	
}