#pragma once
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

		[[nodiscard]] VkCommandBuffer getCommandBuffer() const { return commandBuffer; }
		static constexpr int getNumberOfBuffers() { return MAX_FRAMES_IN_FLIGHT; }
	private:
		void initCommandPool();
		void initCommandBuffers();

		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	};

}

