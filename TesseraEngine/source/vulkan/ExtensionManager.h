#pragma once
#include <vector>

#include "PhysicalDeviceManager.h"

namespace tessera::vulkan
{
	// Instance extensions.
	std::vector<const char*> getRequiredInstanceExtensions();
	void checkIfAllGlsfRequiredExtensionsAreSupported();

	// Logical device extensions.
	std::vector<const char*> getRequiredDeviceExtensions();
	bool isDeviceExtensionSupported(const VkPhysicalDevice& device);
}


