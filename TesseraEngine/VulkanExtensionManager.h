#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanExtensionManager
	{
	public:
		static std::vector<const char*> getRequiredExtensions();
		static void checkIfAllGlsfRequiredExtensionsAreSupported();
	};

}


