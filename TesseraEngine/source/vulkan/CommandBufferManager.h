#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{
	
	class CommandBufferManager final : public Initializable
	{
	public:
		void init() override;

		static void recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, uint32_t imageIndex, const std::shared_ptr<VkRenderPass>& renderPass);

		void clean() override;
	private:
		void initCommandPool();
		void initCommandBuffer();

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	};

}

