#pragma once
#include <vector>

// TODO: Maybe consider using functions instead of methods.

namespace tessera::vulkan
{

	class VulkanExtensionManager
	{
	public:
		static std::vector<const char*> getRequiredExtensions();
		static void checkIfAllGlsfRequiredExtensionsAreSupported();
	};

}


