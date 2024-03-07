#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{
	
	class CommandBufferManager final
	{
	public:
		void init(const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<const VkPhysicalDevice>& physicalDevice, const std::shared_ptr<const VkSurfaceKHR>& surface);
		void clean(const std::shared_ptr<const VkDevice>& device) const;
	private:
		VkCommandPool commandPool = VK_NULL_HANDLE;
	};

}

