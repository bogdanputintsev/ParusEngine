#pragma once
#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{
	
	class CommandBufferManager final : public Initializable
	{
	public:
		void init() override;
		void resetCommandBuffer(const int bufferId) const;
		void clean() override;
		[[nodiscard]] VkCommandBuffer getCommandBuffer(const int bufferId) const;
		[[nodiscard]] VkCommandPool getCommandPool() const { return commandPool; }
		static int getNumberOfBuffers() { return MAX_FRAMES_IN_FLIGHT; }
		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
	private:
		void initCommandPool();
		void initCommandBuffers();

		VkCommandPool commandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> commandBuffers;
	};

	void recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex);
}

