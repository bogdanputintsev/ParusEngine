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

		static void recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex);

		void resetCommandBuffer() const;
		void clean() override;

		[[nodiscard]] std::shared_ptr<VkCommandBuffer> getCommandBuffer() const { return commandBuffer; }
	private:
		void initCommandPool();
		void initCommandBuffer();

		std::shared_ptr<VkCommandPool> commandPool = VK_NULL_HANDLE;
		std::shared_ptr<VkCommandBuffer> commandBuffer = VK_NULL_HANDLE;
	};

}

