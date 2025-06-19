#pragma once
#include <cstdint>
#include <optional>
#include <GLFW/glfw3.h>

#include "PhysicalDeviceManager.h"
#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] inline bool isComplete() const;
	};

	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface);

	class QueueManager final : public Initializable
	{
	public:
		void init() override;
		void drawFrame();
		void clean() override {}

		void onFramebufferResized() { framebufferResized = true; }
		[[nodiscard]] VkQueue getGraphicsQueue() const { return graphicsQueue; }
		[[nodiscard]] VkQueue getPresentQueue() const { return presentQueue; }
	private:
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;

		int currentFrame = 0;
		bool framebufferResized = false;
	};

}

