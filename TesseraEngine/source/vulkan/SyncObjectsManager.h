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
		[[nodiscard]] VkSemaphore getImageAvailableSemaphore() const { return imageAvailableSemaphore; }
		[[nodiscard]] VkSemaphore getRenderFinishedSemaphore() const { return renderFinishedSemaphore; }
		[[nodiscard]] VkFence getInFlightFence() const { return inFlightFence; }
	private:
		VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
		VkFence inFlightFence = VK_NULL_HANDLE;
	};

}

