#pragma once
#include "utils/interfaces/Initializable.h"
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VertexBufferManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkBuffer getVertexBuffer() const { return vertexBuffer; }

	private:
		void createVertexBuffer(const VkDevice& device);
		void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;

		[[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	};
	
}

