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

		void waitForFences(const int currentFrame) const;
		void resetFences(const int currentFrame) const;
		[[nodiscard]] std::vector<VkSemaphore> getImageAvailableSemaphores() const { return imageAvailableSemaphores; }
		[[nodiscard]] std::vector<VkSemaphore> getRenderFinishedSemaphores() const { return renderFinishedSemaphores; }
		[[nodiscard]] std::vector<VkFence> getInFlightFences() const { return inFlightFences; }
	private:
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
	};

}

