#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanValidationLayersManager
	{
	public:
		void checkValidationLayerSupport() const;

		void includeValidationLayerNamesIfNeeded(VkInstanceCreateInfo& createInfo) const;

		static bool isEnabled();
	private:
		const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };
	};

}


