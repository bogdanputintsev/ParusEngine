#pragma once
#include <memory>
#include <memory>
#include <vector>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

#include "DeviceManager.h"

namespace tessera::vulkan
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;

		[[nodiscard]] inline bool isComplete() const { return !formats.empty() && !presentModes.empty(); }
	};

	struct SwapChainImageDetails
	{
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImage> swapChainImages;
	};

	class SwapChainManager final
	{
	public:
		void init(const DeviceManager& deviceManager, const std::shared_ptr<const VkSurfaceKHR>& surface, const std::shared_ptr<GLFWwindow>& window);
		void clean(const std::shared_ptr<const VkDevice>& device) const;
		static SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const std::shared_ptr<const VkSurfaceKHR>& surface);

		[[nodiscard]] SwapChainImageDetails getSwapChainImageDetails() const { return swapChainDetails; }
	private:
		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, const std::shared_ptr<GLFWwindow>& window);

		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		SwapChainImageDetails swapChainDetails {};
	};
	
}

