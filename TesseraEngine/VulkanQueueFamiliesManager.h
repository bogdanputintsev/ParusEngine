#pragma once
#include <cstdint>
#include <optional>

#include "VulkanPhysicalDeviceManager.h"

namespace tessera::vulkan
{

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;

		[[nodiscard]] inline bool isComplete() const;
	};

	class VulkanQueueFamiliesManager final
	{
	public:
		static QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice);
		
	};

}

