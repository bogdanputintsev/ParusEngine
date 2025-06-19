#pragma once
#include <optional>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "entities/Vertex.h"
#include "renderer/Renderer.h"
#include "utils/TesseraLog.h"

namespace tessera::imgui
{
	class ImGuiLibrary;
}

namespace tessera::vulkan
{
	struct SwapChainSupportDetails
	{
		VkSurfaceCapabilitiesKHR capabilities{};
		std::vector<VkSurfaceFormatKHR> formats{};
		std::vector<VkPresentModeKHR> presentModes{};

		[[nodiscard]] bool isComplete() const { return !formats.empty() && !presentModes.empty(); }
	};

	struct SwapChainImageDetails
	{
		VkFormat swapChainImageFormat{};
		VkExtent2D swapChainExtent{};
		std::vector<VkImage> swapChainImages{};
	};

	struct QueueFamilyIndices
	{
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		[[nodiscard]] bool isComplete() const { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	struct Texture
	{
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		uint32_t maxMipLevels;
	};

	struct Model
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		Texture texture;
		std::vector<VkDescriptorSet> descriptorSets;
		size_t vertexOffset; // Offset into the combined vertex buffer
		size_t indexOffset;  // Offset into the combined index buffer
	};

	struct VulkanContext final
	{
		// Load model
		Model loadModel(const std::string& modelPath, const std::string& texturePath);

		// Instance
		void createInstance();
		static void checkValidationLayerSupport();
		static bool validationLayersAreEnabled();
		static std::vector<const char*> getValidationLayers();
		static void checkIfAllGlsfRequiredExtensionsAreSupported();
		void destroyDebugUtilsMessengerExt(VkDebugUtilsMessengerEXT debugMessengerToDestroy,
			const VkAllocationCallbacks* pAllocator) const;
		static std::vector<const char*> getRequiredInstanceExtensions();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			[[maybe_unused]] void* pUserData);
		static void populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		static LogType getLogType(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity);

		// DebugManager
		void createDebugManager();
		VkResult createDebugUtilsMessengerExt(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
			const VkAllocationCallbacks* pAllocator,
			VkDebugUtilsMessengerEXT* pDebugMessenger) const;

		// Surface.
		void createSurface();

		// Devices.
		void createDevices();
		void pickAnySuitableDevice();
		static bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);
		static SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

		// Queues
		void createQueues();

		// SwapChain
		void createSwapChain();
		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		[[nodiscard]] std::optional<uint32_t> acquireNextImage();
		void recreateSwapChain();
		void cleanupSwapChain() const;

		// Image View
		void createImageViews();
		VkImageView createImageView(const VkImage image, const VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;

		// Render Pass
		void createRenderPass();

		// Descriptor Set Layout
		void createDescriptorSetLayout();

		// Graphics Pipeline
		void createGraphicsPipeline();
		static VkShaderModule createShaderModule(const std::vector<char>& code, const VkDevice& device);

		// Framebuffer
		void createFramebuffers();

		// Command buffer
		void createCommandBuffer();
		void resetCommandBuffer(const int bufferId) const;
		void recordCommandBuffer(VkCommandBuffer commandBufferToRecord, uint32_t imageIndex) const;
		[[nodiscard]] VkCommandBuffer getCommandBuffer(const int bufferId) const;
		[[nodiscard]] VkCommandPool getCommandPool() const { return commandPool; }

		// Command pool
		void createCommandPool();
		VkCommandBuffer beginSingleTimeCommands() const;
		void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;

		// Depth resources
		void createDepthResources();
		VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		VkFormat findDepthFormat();
		static bool hasStencilComponent(VkFormat format);

		// Texture image
		Texture loadTexture(const std::string& texturePath);
		void generateMipmaps(const Texture& texture, VkFormat imageFormat,
		                     int32_t texWidth,
		                     int32_t texHeight) const;
		void createImage(uint32_t width, uint32_t height, uint32_t numberOfMipLevels, VkSampleCountFlagBits numberOfSamples, VkFormat format, VkImageTiling
			tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags
			properties, VkImage& image, VkDeviceMemory& imageMemory) const;
		void transitionImageLayout(const VkImage image, VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout, uint32_t mipLevels) const;
		void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) const;
		void createTextureImageView(Texture& texture) const;

		void createTextureImageView(const Texture& textureImage);
		VkSampler createTextureSampler(uint32_t maxMipLevels) const;
		void createColorResources();

		// Buffer manager
		void createBufferManager();
		void createVertexBuffer(const std::vector<Vertex>& vertices);
		void createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
		[[nodiscard]] uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
		void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
		void createIndexBuffer(const std::vector<uint32_t>& indices);
		void createUniformBuffer();
		void updateUniformBuffer(uint32_t currentImage) const;

		void createDescriptorPool();
		void createDescriptorSets(Model& model) const;

		// Sync objects
		void createSyncObjects();
		void waitForFences() const;
		void resetFences() const;

		void onResize();

		VkSampleCountFlagBits getMaxUsableSampleCount() const;

		static VkVertexInputBindingDescription getBindingDescription();

		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

		VkInstance instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT debugMessenger = nullptr;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		SwapChainImageDetails swapChainDetails{};
		std::vector<VkImageView> swapChainImageViews{};
		VkDescriptorPool descriptorPool;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		std::vector<VkFramebuffer> swapChainFramebuffers{};
		std::vector<VkCommandBuffer> commandBuffers{};
		VkCommandPool commandPool = VK_NULL_HANDLE;
		VkBuffer vertexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
		VkBuffer indexBuffer = VK_NULL_HANDLE;
		VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
		
		std::vector<Model> models;
		
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkImage colorImage;
		VkDeviceMemory colorImageMemory;
		VkImageView colorImageView;

		std::vector<VkBuffer> uniformBuffers{};
		std::vector<VkDeviceMemory> uniformBuffersMemory{};
		std::vector<void*> uniformBuffersMapped{};
		std::vector<VkSemaphore> imageAvailableSemaphores{};
		std::vector<VkSemaphore> renderFinishedSemaphores{};
		std::vector<VkFence> inFlightFences{};

		int currentFrame = 0;
		bool framebufferResized = false;

		static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
		static constexpr int IMAGE_SAMPLER_POOL_SIZE = 1000;
		static constexpr uint32_t DESCRIPTOR_SET_COUNT = MAX_FRAMES_IN_FLIGHT + IMAGE_SAMPLER_POOL_SIZE;

	};

	class VulkanRenderer final : public Renderer
	{
	public:
		void init() override;
		void clean() override;
		void drawFrame() override;
		void deviceWaitIdle() override;

		friend class tessera::imgui::ImGuiLibrary;
	private:
		VulkanContext context;
	};

}

