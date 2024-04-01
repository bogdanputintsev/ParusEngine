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
		VkBufferCreateInfo createVertexBuffer(const VkDevice& device);
		void allocateVertexBufferMemory(const VkDevice& device);
		void fillVertexBufferData(const VkDevice& device, const VkBufferCreateInfo& bufferInfo) const;

		[[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
	};
	
}

