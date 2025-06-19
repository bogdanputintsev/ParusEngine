#include "VertexBufferManager.h"

#include "entities/Vertex.h"
#include "utils/interfaces/ServiceLocator.h"
#include "vulkan/DeviceManager.h"

namespace tessera::vulkan
{

	void VertexBufferManager::init()
	{
		Initializable::init();
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		const VkBufferCreateInfo bufferInfo = createVertexBuffer(device);
		allocateVertexBufferMemory(device);

		vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);

		fillVertexBufferData(device, bufferInfo);
	}

	VkBufferCreateInfo VertexBufferManager::createVertexBuffer(const VkDevice& device)
	{
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();
		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		bufferInfo.flags = 0;

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) 
		{
			throw std::runtime_error("VertexBufferManager: failed to create vertex buffer.");
		}

		return bufferInfo;
	}

	void VertexBufferManager::allocateVertexBufferMemory(const VkDevice& device)
	{
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("VertexBufferManager: failed to allocate vertex buffer memory.");
		}
	}

	void VertexBufferManager::fillVertexBufferData(const VkDevice& device, const VkBufferCreateInfo& bufferInfo) const
	{
		void* data;
		vkMapMemory(device, vertexBufferMemory, 0, bufferInfo.size, 0, &data);
		memcpy(data, vertices.data(), bufferInfo.size);
		vkUnmapMemory(device, vertexBufferMemory);
	}

	uint32_t VertexBufferManager::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
	{
		const auto& physicalDevice = ServiceLocator::getService<DeviceManager>()->getPhysicalDevice();

		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) 
		{
			if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("VertexBufferManager: failed to find suitable memory type.");
	}

	void VertexBufferManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vkFreeMemory(device, vertexBufferMemory, nullptr);
	}
}
