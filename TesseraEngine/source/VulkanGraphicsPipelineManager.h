#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanGraphicsPipelineManager
	{
	public:
		void init(const std::shared_ptr<const VkDevice>& device);
		void clean(const std::shared_ptr<const VkDevice>& device) const;
	private:
		static VkShaderModule createShaderModule(const std::vector<char>& code, const std::shared_ptr<const VkDevice>& device);

		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
	};
	
}

