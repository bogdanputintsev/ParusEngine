#pragma once
#include <vector>

#include "VulkanPhysicalDeviceManager.h"

// TODO: Maybe consider using functions instead of methods.

namespace tessera::vulkan
{

	class VulkanExtensionManager
	{
	public:
		// Instance extensions.
		static std::vector<const char*> getRequiredInstanceExtensions();
		static void checkIfAllGlsfRequiredExtensionsAreSupported();

		// Logical device extensions.
		static std::vector<const char*> getRequiredDeviceExtensions();
		static bool isDeviceExtensionSupported(const VkPhysicalDevice& device);
	};

}


