#pragma once
#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class SyncObjectsManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		void waitForFences() const;
		[[nodiscard]] std::shared_ptr<const VkSemaphore> getImageAvailableSemaphore() const { return imageAvailableSemaphore; }
		[[nodiscard]] std::shared_ptr<const VkSemaphore> getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }
		[[nodiscard]] std::shared_ptr<const VkFence> getInFlightFence() const { return inFlightFence; }
	private:
		std::shared_ptr<VkSemaphore> imageAvailableSemaphore = VK_NULL_HANDLE;
		std::shared_ptr<VkSemaphore> renderFinishedSemaphore = VK_NULL_HANDLE;
		std::shared_ptr<VkFence> inFlightFence = VK_NULL_HANDLE;
	};

}

