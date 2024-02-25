#pragma once
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include "VulkanExtensionManager.h"
#include "VulkanValidationLayersManager.h"

namespace tessera::vulkan
{
	class VulkanInitializer final
	{
	public:
		VulkanInitializer() = default;
		void init();
		void clean() const;

	private:
		void createInstance();
		static void checkIfAllRequiredExtensionsAreSupported(const char** requiredExtensions, const uint32_t requiredExtensionCount);

		VkInstance instance = nullptr;
		VulkanExtensionManager extensionManager;
		VulkanValidationLayersManager validationLayersManager;
	};
}


