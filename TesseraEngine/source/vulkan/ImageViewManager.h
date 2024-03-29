#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "SwapChainManager.h"

namespace tessera::vulkan
{

	class ImageViewManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] std::vector<VkImageView> getSwapChainImageViews() const { return swapChainImageViews; }
	private:
		std::vector<VkImageView> swapChainImageViews;
	};

}

