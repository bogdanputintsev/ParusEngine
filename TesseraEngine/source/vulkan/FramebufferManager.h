#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "SwapChainManager.h"

namespace tessera::vulkan
{
	
	class FramebufferManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] std::vector<VkFramebuffer> getSwapChainFramebuffers() const { return swapChainFramebuffers; }

	private:
		std::vector<VkFramebuffer> swapChainFramebuffers;
	};

}

