#include "VulkanRenderer.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <GLFW/glfw3.h>

#include "Core.h"
#include "entities/UniformBufferObject.h"
#include "graphics/glfw/GlfwLibrary.h"
#include "utils/ShaderLoader.h"
#include "utils/TesseraLog.h"

namespace tessera::vulkan
{

	void VulkanContext::createInstance()
	{
		checkValidationLayerSupport();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Sandbox";
		appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		appInfo.pEngineName = "Tessera Engine";
		appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		checkIfAllGlsfRequiredExtensionsAreSupported();

		const std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		const std::vector<const char*> validationLayers = getValidationLayers();
		if (validationLayersAreEnabled())
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populate(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanInstanceManager: failed to create instance.");
		}
	}

	void VulkanContext::checkValidationLayerSupport()
	{
		if (!validationLayersAreEnabled())
		{
			return;
		}

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool result = true;

		for (const char* layerName : getValidationLayers())
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				result = false;
				break;
			}
		}

		if (!result)
		{
			throw std::runtime_error("VulkanInitializer: validation layers requested, but not available.");
		}
	}

	bool VulkanContext::validationLayersAreEnabled()
	{
#ifdef NDEBUG
		return false;
#else
		return true;
#endif
	}

	std::vector<const char*> VulkanContext::getValidationLayers()
	{
		return { "VK_LAYER_KHRONOS_validation" };
	}

	void VulkanContext::checkIfAllGlsfRequiredExtensionsAreSupported()
	{
		const std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
		const std::unordered_set<std::string> requiredExtensionsSet{ requiredExtensions.begin(), requiredExtensions.end() };

		uint32_t matches = 0;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		for (const auto& [extensionName, specVersion] : extensions)
		{
			// TODO: Custom setting for printing extensions.
			//std::cout << extensionName << " " << requiredExtensionsSet.contains(extensionName) << std::endl;

			if (requiredExtensionsSet.contains(std::string(extensionName)))
			{
				++matches;
			}
		}

		if (matches != requiredExtensions.size())
		{
			throw std::runtime_error("VulkanInitializer: GLFW window extension is not supported.");
		}
	}

	void VulkanContext::destroyDebugUtilsMessengerExt(VkDebugUtilsMessengerEXT debugMessengerToDestroy, const VkAllocationCallbacks* pAllocator) const
	{
		const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (func != nullptr)
		{
			func(instance, debugMessengerToDestroy, pAllocator);
		}
	}

	std::vector<const char*> VulkanContext::getRequiredInstanceExtensions()
	{
		std::vector extensions = CORE->graphicsLibrary->getRequiredExtensions();

		if (validationLayersAreEnabled())
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VkBool32 VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData)
	{
		TesseraLog::send(getLogType(messageSeverity), "Vulkan", pCallbackData->pMessage);
		return VK_FALSE;
	}

	void VulkanContext::populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	LogType VulkanContext::getLogType(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return LogType::DEBUG;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return LogType::INFO;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return LogType::WARNING;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return LogType::ERROR;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			return LogType::INFO;
		}

		return LogType::INFO;
	}

	void VulkanContext::createDebugManager()
	{
		if (!validationLayersAreEnabled())
		{
			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		populate(createInfo);

		if (createDebugUtilsMessengerExt(&createInfo, nullptr, &debugMessenger) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanDebugManager: failed to set up debug messenger!");
		}
	}

	VkResult VulkanContext::createDebugUtilsMessengerExt(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) const
	{
		const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
		if (func != nullptr)
		{
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}

		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}

	void VulkanContext::createSurface()
	{
		const std::shared_ptr<GLFWwindow>& window = CORE->graphicsLibrary->getWindow();

		if (glfwCreateWindowSurface(instance, window.get(), nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanSurfaceManager: failed to create window surface.");
		}
	}

	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
	{
		QueueFamilyIndices familyIndices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				familyIndices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			if (presentSupport)
			{
				familyIndices.presentFamily = i;
			}

			if (familyIndices.isComplete())
			{
				break;
			}
		}

		return familyIndices;
	}

	// Logical device extensions.
	std::vector<const char*> getRequiredDeviceExtensions()
	{
		return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	}

	bool isDeviceExtensionSupported(const VkPhysicalDevice& device)
	{
		const std::vector<const char*> requiredExtensions = getRequiredDeviceExtensions();

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// TODO: Optimize with O(1) complexity.
		std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

		for (const auto& extension : availableExtensions)
		{
			requiredExtensionsSet.erase(extension.extensionName);
		}

		return requiredExtensionsSet.empty();
	}

	void VulkanContext::createDevices()
	{
		pickAnySuitableDevice();

		// Create logical device.
		assert(physicalDevice);

		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);
		if (!graphicsFamily.has_value() || !presentFamily.has_value())
		{
			throw std::runtime_error("VulkanLogicalDeviceManager: queue family indices are not complete.");
		}

		std::set uniqueQueueFamilies = { graphicsFamily.value(), presentFamily.value() };

		constexpr float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.emplace_back(queueCreateInfo);
		}

		// Specifying used device features.
		constexpr VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;

		const auto requiredExtensions = getRequiredDeviceExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		// Distinction between instance and device specific validation no longer the case. This was added for back compatibility.
		const std::vector<const char*> validationLayers = getValidationLayers();
		if (validationLayersAreEnabled())
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanLogicalDeviceManager: failed to create logical device.");
		}
	}

	void VulkanContext::pickAnySuitableDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0)
		{
			throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices)
		{
			if (isDeviceSuitable(device, surface))
			{
				physicalDevice = device;
				return;
			}
		}

		throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find a suitable GPU!");
	}

	bool VulkanContext::isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		// Basic device properties like the name, type and supported Vulkan version.
		[[maybe_unused]] VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// The support for optional features like texture compression, 64 bit floats and multi viewport rendering.
		[[maybe_unused]] VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// Check if device can process the commands we want to use.
		const QueueFamilyIndices indices = findQueueFamilies(device, surface);

		// Check if physical device supports swap chain extension.
		const bool extensionsSupported = isDeviceExtensionSupported(device);

		// Check if physical device supports swap chain.
		const SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);

		return indices.isComplete() && extensionsSupported && swapChainSupport.isComplete();
	}

	SwapChainSupportDetails VulkanContext::querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
	{
		SwapChainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	void VulkanContext::createQueues()
	{
		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);

		if (!graphicsFamily.has_value() || !presentFamily.has_value())
		{
			throw std::runtime_error("Queue Family is undefined.");
		}

		vkGetDeviceQueue(logicalDevice, graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, presentFamily.value(), 0, &presentQueue);
	}

	void VulkanContext::createSwapChain()
	{
		const auto [capabilities, formats, presentModes] = querySwapChainSupport(physicalDevice, surface);

		const auto [format, colorSpace] = chooseSwapSurfaceFormat(formats);
		const VkPresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
		const VkExtent2D extent = chooseSwapExtent(capabilities);

		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = format;
		createInfo.imageColorSpace = colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);
		const uint32_t queueFamilyIndices[] = { graphicsFamily.value(), presentFamily.value() };

		if (graphicsFamily != presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}

		createInfo.preTransform = capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanSwapChainManager: failed to create swap chain.");
		}

		std::vector<VkImage> swapChainImages;
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &imageCount, swapChainImages.data());

		swapChainDetails = { format, extent, swapChainImages };
	}

	VkSurfaceFormatKHR VulkanContext::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		assert(!availableFormats.empty());

		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	VkPresentModeKHR VulkanContext::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		// TODO: Replace with std::find.
		for (const auto& availablePresentMode : availablePresentModes)
		{
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				return availablePresentMode;
			}
		}

		// Only the VK_PRESENT_MODE_FIFO_KHR mode is guaranteed to be available.
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D VulkanContext::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}

		int width, height;
		glfwGetFramebufferSize(CORE->graphicsLibrary->getWindow().get(), &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return actualExtent;
	}

	std::optional<uint32_t> VulkanContext::acquireNextImage()
	{
		if (static_cast<size_t>(currentFrame) >= imageAvailableSemaphores.size() || currentFrame < 0)
		{
			throw std::out_of_range("SyncObjectsManager: current frame number is larger than number of fences.");
		}

		uint32_t imageIndex;
		const VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return std::nullopt;
		}

		if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("SyncObjectsManager: failed to acquire swap chain image.");
		}

		return imageIndex;
	}

	void VulkanContext::recreateSwapChain()
	{
		CORE->graphicsLibrary->handleMinimization();
		CORE->renderer->deviceWaitIdle();

		cleanupSwapChain();

		createSwapChain();
		createImageView();
		createFramebuffer();
	}

	void VulkanContext::cleanupSwapChain()
	{
		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);
		}

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	void VulkanContext::createImageView()
	{
		const auto& [swapChainImageFormat, swapChainExtent, swapChainImages]
			= swapChainDetails;

		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			if (vkCreateImageView(logicalDevice, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("VulkanImageViewManager: failed to create image views.");
			}
		}
	}

	void VulkanContext::createRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainDetails.swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void VulkanContext::createDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("DescriptorSetLayoutManager: failed to create descriptor set layout!");
		}
	}

	void VulkanContext::createGraphicsPipeline()
	{
		const auto vertexShaderCode = ShaderLoader::readFile("shaders/vert.spv");
		vertexShaderModule = createShaderModule(vertexShaderCode, logicalDevice);

		const auto fragmentShaderCode = ShaderLoader::readFile("shaders/frag.spv");
		fragmentShaderModule = createShaderModule(fragmentShaderCode, logicalDevice);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		[[maybe_unused]] VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// Vertex input
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// Input assembly
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		//// Viewport.
		//VkViewport viewport{};

		//const auto& [width, height] = swapChainDetails.swapChainExtent;
		//viewport.x = 0.0f;
		//viewport.y = 0.0f;
		//viewport.width = static_cast<float>(width);
		//viewport.height = static_cast<float>(height);
		//viewport.minDepth = 0.0f;
		//viewport.maxDepth = 1.0f;

		//// Scissor.
		//VkRect2D scissor{};
		//scissor.offset = { 0, 0 };
		//scissor.extent = swapChainDetails.swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		//viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		//viewportState.pScissors = &scissor;

		// Rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// Multisampling
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f;
		multisampling.pSampleMask = nullptr;
		multisampling.alphaToCoverageEnable = VK_FALSE;
		multisampling.alphaToOneEnable = VK_FALSE;

		// Depth and stencil testing. Not used for now.
		[[maybe_unused]] VkPipelineDepthStencilStateCreateInfo depthStencil{};

		// Color blending.
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// Dynamic state
		std::vector dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// Pipeline layout.
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0;
		pipelineLayoutInfo.pPushConstantRanges = nullptr;

		if (vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanGraphicsPipelineManager: failed to create pipeline layout.");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanGraphicsPipelineManager: failed to create graphics pipeline.");
		}
	}

	VkShaderModule VulkanContext::createShaderModule(const std::vector<char>& code, const VkDevice& device)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanGraphicsPipelineManager: failed to create shader module.");
		}

		return shaderModule;
	}

	void VulkanContext::createFramebuffer()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

		for (size_t i = 0; i < swapChainImageViews.size(); ++i)
		{
			const VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			const auto& [width, height] = swapChainDetails.swapChainExtent;

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("VulkanFramebufferManager: failed to create framebuffer.");
			}
		}
	}

	void VulkanContext::createCommandBuffer()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		if (vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to allocate command buffers.");
		}
	}

	void VulkanContext::resetCommandBuffer(const int bufferId) const
	{
		if (static_cast<size_t>(bufferId) >= commandBuffers.size() || bufferId < 0)
		{
			throw std::out_of_range("VkCommandBuffer: current frame number is larger than number of fences.");
		}
		vkResetCommandBuffer(commandBuffers[bufferId], 0);
	}

	void VulkanContext::recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex) const
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBufferToRecord, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to begin recording command buffer.");
		}

		// Start the rendering pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainDetails.swapChainExtent;
		renderPassInfo.clearValueCount = 1;
		constexpr VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.pClearValues = &clearColor;

		// Draw.
		vkCmdBeginRenderPass(commandBufferToRecord, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		// TODO: Maybe RAII initialization?
		// Bind the graphics pipeline.
		vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainDetails.swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainDetails.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBufferToRecord, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainDetails.swapChainExtent;
		vkCmdSetScissor(commandBufferToRecord, 0, 1, &scissor);

		const VkBuffer vertexBuffers[] = { vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBufferToRecord, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBufferToRecord, indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindDescriptorSets(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);

		const auto numberOfIndices = static_cast<uint32_t>(indices.size());

		vkCmdDrawIndexed(commandBufferToRecord, numberOfIndices, 1, 0, 0, 0);
		vkCmdEndRenderPass(commandBufferToRecord);

		if (vkEndCommandBuffer(commandBufferToRecord) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to record command buffer");
		}
	}

	VkCommandBuffer VulkanContext::getCommandBuffer(const int bufferId) const
	{
		if (static_cast<size_t>(bufferId) >= commandBuffers.size() || bufferId < 0)
		{
			throw std::out_of_range("VkCommandBuffer: current frame number is larger than number of fences.");
		}
		return commandBuffers[bufferId];
	}

	void VulkanContext::createCommandPool()
	{
		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = graphicsFamily.value();

		if (vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to create command pool.");
		}
	}

	void VulkanContext::createBufferManager()
	{
		createVertexBuffer();
		createIndexBuffer();
		createUniformBuffer();
	}

	void VulkanContext::createVertexBuffer()
	{
		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// Fill vertex buffer data.
		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
		copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanContext::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
	{
		// Create buffer structure.
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		if (vkCreateBuffer(logicalDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
		{
			throw std::runtime_error("VertexBufferManager: failed to create buffer.");
		}

		// Calculate memory requirements.
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(logicalDevice, buffer, &memRequirements);

		// Allocate vertex buffer memory.
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

		if (vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
		{
			throw std::runtime_error("VertexBufferManager: failed to allocate buffer memory.");
		}

		vkBindBufferMemory(logicalDevice, buffer, bufferMemory, 0);
	}

	uint32_t VulkanContext::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties) const
	{
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
		{
			if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}

		throw std::runtime_error("VertexBufferManager: failed to find suitable memory type.");
	}

	void VulkanContext::copyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size) const
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VkBufferCopy copyRegion;
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue);
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	void VulkanContext::createIndexBuffer()
	{
		const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		void* data;
		vkMapMemory(logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(logicalDevice, stagingBufferMemory);

		createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

		copyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vkDestroyBuffer(logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanContext::createUniformBuffer()
	{
		constexpr VkDeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

			vkMapMemory(logicalDevice, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
		}
	}

	void VulkanContext::updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), swapChainDetails.swapChainExtent.width / (float)swapChainDetails.swapChainExtent.height, 0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
	}

	void VulkanContext::createDescriptorPool()
	{
		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void VulkanContext::createDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = descriptorSets[i];
			descriptorWrite.dstBinding = 0;
			descriptorWrite.dstArrayElement = 0;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(logicalDevice, 1, &descriptorWrite, 0, nullptr);
		}
	}

	void VulkanContext::createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			if (vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("SyncObjectsManager: failed to create semaphores.");
			}
		}
	}

	void VulkanContext::waitForFences() const
	{
		if (static_cast<size_t>(currentFrame) >= inFlightFences.size() || currentFrame < 0)
		{
			throw std::out_of_range("SyncObjectsManager: current frame number is larger than number of fences.");
		}

		vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
	}

	void VulkanContext::resetFences() const
	{
		if (static_cast<size_t>(currentFrame) >= inFlightFences.size() || currentFrame < 0)
		{
			throw std::out_of_range("SyncObjectsManager: current frame number is larger than number of fences.");
		}

		vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
	}


	// ===========================================================================================]
	void VulkanRenderer::init()
	{
		context.createInstance();
		context.createDebugManager();
		context.createSurface();
		context.createDevices();
		context.createQueues();
		context.createSwapChain();
		context.createImageView();
		context.createRenderPass();
		context.createDescriptorSetLayout();
		context.createGraphicsPipeline();
		context.createFramebuffer();
		context.createCommandPool();
		context.createBufferManager();
		context.createDescriptorPool();
		context.createDescriptorSets();
		context.createCommandBuffer();
		context.createSyncObjects();
	}

	void VulkanRenderer::clean()
	{
		context.cleanupSwapChain();

		vkDestroyPipeline(context.logicalDevice, context.graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(context.logicalDevice, context.pipelineLayout, nullptr);
		vkDestroyRenderPass(context.logicalDevice, context.renderPass, nullptr);

		for (size_t i = 0; i < VulkanContext::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(context.logicalDevice, context.uniformBuffers[i], nullptr);
			vkFreeMemory(context.logicalDevice, context.uniformBuffersMemory[i], nullptr);
		}

		vkDestroyDescriptorPool(context.logicalDevice, context.descriptorPool, nullptr);
		vkDestroyDescriptorSetLayout(context.logicalDevice, context.descriptorSetLayout, nullptr);

		vkDestroyBuffer(context.logicalDevice, context.indexBuffer, nullptr);
		vkFreeMemory(context.logicalDevice, context.indexBufferMemory, nullptr);

		vkDestroyBuffer(context.logicalDevice, context.vertexBuffer, nullptr);
		vkFreeMemory(context.logicalDevice, context.vertexBufferMemory, nullptr);

		for (size_t i = 0; i < VulkanContext::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(context.logicalDevice, context.renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(context.logicalDevice, context.imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(context.logicalDevice, context.inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(context.logicalDevice, context.commandPool, nullptr);
		vkDestroyDevice(context.logicalDevice, nullptr);

		if (context.validationLayersAreEnabled())
		{
			context.destroyDebugUtilsMessengerExt(context.debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(context.instance, context.surface, nullptr);
		vkDestroyInstance(context.instance, nullptr);
	}

	void VulkanRenderer::drawFrame()
	{
		vkWaitForFences(context.logicalDevice, 1, &context.inFlightFences[context.currentFrame], VK_TRUE, UINT64_MAX);

		const std::optional<uint32_t> imageIndex = context.acquireNextImage();
		if (!imageIndex.has_value())
		{
			return;
		}

		context.updateUniformBuffer(context.currentFrame);
		vkResetFences(context.logicalDevice, 1, &context.inFlightFences[context.currentFrame]);

		const auto commandBuffer = context.getCommandBuffer(context.currentFrame);

		context.resetCommandBuffer(context.currentFrame);
		context.recordCommandBuffer(commandBuffer, imageIndex.value());

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = { context.imageAvailableSemaphores[context.currentFrame] };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		const VkSemaphore signalSemaphores[] = { context.renderFinishedSemaphores[context.currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(context.graphicsQueue, 1, &submitInfo, context.inFlightFences[context.currentFrame]) != VK_SUCCESS)
		{
			throw std::runtime_error("QueueManager: failed to submit draw command buffer.");
		}

		// Submit result back to swap chain to have it eventually show up on the screen. 
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swapChains[] = { context.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex.value();
		presentInfo.pResults = nullptr;

		const VkResult result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || context.framebufferResized)
		{
			context.framebufferResized = false;
			context.recreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("QueueManager: failed to present swap chain image.");
		}

		context.currentFrame = (context.currentFrame + 1) % VulkanContext::MAX_FRAMES_IN_FLIGHT;
	}

	void VulkanRenderer::deviceWaitIdle()
	{
		vkDeviceWaitIdle(context.logicalDevice);
	}

	void VulkanRenderer::onResize()
	{
		context.framebufferResized = true;
	}
}
