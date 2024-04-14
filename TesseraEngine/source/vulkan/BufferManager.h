#pragma once
#include "utils/interfaces/Initializable.h"
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class BufferManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkBuffer getVertexBuffer() const { return vertexBuffer; }
		[[nodiscard]] VkBuffer getIndexBuffer() const { return indexBuffer; }
	private:
		void createVertexBuffer(const VkDevice& device);
		void createIndexBuffer(const VkDevice& device);
		void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;

		[[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
		VkBuffer indexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
	};
	
}

