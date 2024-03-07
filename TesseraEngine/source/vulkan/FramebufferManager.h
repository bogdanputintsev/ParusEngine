#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "SwapChainManager.h"

namespace tessera::vulkan
{
	
	class FramebufferManager final
	{
	public:
		void init(const std::vector<VkImageView>& swapChainImageViews, const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<VkRenderPass>& renderPass, const SwapChainImageDetails& swapChainImageDetails);
		void clean(const std::shared_ptr<const VkDevice>& device) const;
	private:
		std::vector<VkFramebuffer> swapChainFramebuffers;
	};

}

