#pragma once
#include <cstdint>
#include <memory>
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

	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const std::shared_ptr<const VkSurfaceKHR>& surface);

	class QueueManager final : public Initializable
	{
	public:
		void init() override;
		void drawFrame() const;
		void clean() override {}
		[[nodiscard]] std::shared_ptr<const VkQueue> getGraphicsQueue() const { return graphicsQueue; }
		[[nodiscard]] std::shared_ptr<const VkQueue> getPresentQueue() const { return presentQueue; }
	private:
		std::shared_ptr<VkQueue> graphicsQueue = VK_NULL_HANDLE;
		std::shared_ptr<VkQueue> presentQueue = VK_NULL_HANDLE;
	};

}

