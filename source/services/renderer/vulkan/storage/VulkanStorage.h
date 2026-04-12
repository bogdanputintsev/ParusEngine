#pragma once


#include <array>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <mutex>

#include "services/renderer/vulkan/utils/VulkanUtils.h"


namespace parus::vulkan
{
	class VulkanTexture2d;

	struct GlobalGeometryBuffers
    {
        VkBuffer vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer skyVertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory skyVertexBufferMemory = VK_NULL_HANDLE;
        VkBuffer indexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
        VkBuffer skyIndexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory skyIndexBufferMemory = VK_NULL_HANDLE;
        size_t totalVertices = 0;
        size_t totalSkyVertices = 0;
        size_t totalIndices = 0;
        size_t totalSkyIndices = 0;
    };

	struct UboBuffer
    {
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        std::array<VkBuffer, MAX_FRAMES_IN_FLIGHT> frameBuffers{};
        std::array<VkDeviceMemory, MAX_FRAMES_IN_FLIGHT> memory{};
        std::array<void*, MAX_FRAMES_IN_FLIGHT> mapped{};
    };

	struct VulkanStorage final
    {
        static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
        std::mutex graphicsQueueMutex;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		utils::SwapChainImageDetails swapChainDetails{};
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

    	// Descriptors
		VkDescriptorPool descriptorPool;

    	// Framebuffers
    	std::vector<VkFramebuffer> swapChainFramebuffers{};

    	// UBO Buffers
    	UboBuffer globalUboBuffer{};
    	UboBuffer instanceUboBuffer{};
    	UboBuffer directionalLightUboBuffer{};
    	UboBuffer pointLightUboBuffer{};

    	// Geometry Buffers
    	GlobalGeometryBuffers globalBuffers{};

    	// Global Descriptor Sets
    	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> globalDescriptorSets{};

    	// Command Buffers
    	std::vector<VkCommandBuffer> commandBuffers{};

		// Command Pools
		std::unordered_map<std::thread::id, VkCommandPool> threadCommandPools;
		
    	// Sync Objects
    	std::vector<VkSemaphore> imageAvailableSemaphores{};
    	std::vector<VkSemaphore> renderFinishedSemaphores{};
    	std::vector<VkFence> inFlightFences{};

    };

}
