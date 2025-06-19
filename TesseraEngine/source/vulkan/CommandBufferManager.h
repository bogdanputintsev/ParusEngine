#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{
	
	class CommandBufferManager final
	{
	public:
		void init(const std::shared_ptr<const VkDevice>& device,
		          const std::shared_ptr<const VkPhysicalDevice>& physicalDevice,
		          const std::shared_ptr<const VkSurfaceKHR>& surface, const std::shared_ptr<VkRenderPass>& renderPass);

		static void recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, uint32_t imageIndex, const std::shared_ptr<VkRenderPass>& renderPass);

		void clean(const std::shared_ptr<const VkDevice>& device) const;
	private:
		void initCommandPool(const std::shared_ptr<const VkDevice>& device,
			const std::shared_ptr<const VkPhysicalDevice>& physicalDevice,
			const std::shared_ptr<const VkSurfaceKHR>& surface);
		void initCommandBuffer(const std::shared_ptr<const VkDevice>& device);

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	};

}

