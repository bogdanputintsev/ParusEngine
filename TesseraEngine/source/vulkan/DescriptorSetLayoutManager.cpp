#include "DescriptorSetLayoutManager.h"

#include "DeviceManager.h"
#include "utils/interfaces/ServiceLocator.h"


namespace tessera::vulkan
{

	void DescriptorSetLayoutManager::init()
	{
		Initializable::init();

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) 
		{
			throw std::runtime_error("DescriptorSetLayoutManager: failed to create descriptor set layout!");
		}
	}

	void DescriptorSetLayoutManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
	}
}
