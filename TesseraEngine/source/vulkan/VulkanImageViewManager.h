#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "VulkanSwapChainManager.h"

namespace tessera::vulkan
{

	class VulkanImageViewManager final
	{
	public:
		void init(const SwapChainImageDetails& swapChainImageDetails, const std::shared_ptr<const VkDevice>& device);
		void clean(const std::shared_ptr<const VkDevice>& device) const;

		[[nodiscard]] std::vector<VkImageView> getSwapChainImageViews() const { return swapChainImageViews; }
	private:
		std::vector<VkImageView> swapChainImageViews;
	};

}

