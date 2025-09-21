#include "VkDeviceBuilder.h"

#include <set>
#include <unordered_set>

#include "engine/EngineCore.h"
#include "services/renderer/vulkan/VulkanRenderer.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    void VkDeviceBuilder::build(VulkanStorage& storage)
    {
        pickAnySuitableDevice(storage);
 
        // Create logical device.
		ASSERT(storage.physicalDevice, "Devices hasn't been picked successfully.");
 
		const auto [graphicsFamily, presentFamily] = utils::findQueueFamilies(storage.physicalDevice, storage.surface);
		ASSERT(graphicsFamily.has_value() && presentFamily.has_value(), "Queue family indices are not complete.");
 
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
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
 
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(REQUIRED_DEVICE_EXTENSIONS.size());
		createInfo.ppEnabledExtensionNames = REQUIRED_DEVICE_EXTENSIONS.data();
 
		// Distinction between instance and device specific validation no longer the case. This was added for back compatibility.
		if constexpr (utils::validationLayersEnabled())
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(utils::REQUIRED_VALIDATION_LAYERS.size());
			createInfo.ppEnabledLayerNames = utils::REQUIRED_VALIDATION_LAYERS.data();
		}
		else
		{
			createInfo.enabledLayerCount = 0;
		}
 
		ASSERT(vkCreateDevice(storage.physicalDevice, &createInfo, nullptr, &storage.logicalDevice) == VK_SUCCESS, "failed to create logical device.");

        utils::setDebugName(storage, storage.instance, VK_OBJECT_TYPE_INSTANCE, "Main Vulkan Instance");
    	utils::setDebugName(storage, storage.physicalDevice, VK_OBJECT_TYPE_PHYSICAL_DEVICE, "Main Physical Device");
    	utils::setDebugName(storage, storage.logicalDevice, VK_OBJECT_TYPE_DEVICE, "Main Logical Device");
    }
 
    void VkDeviceBuilder::pickAnySuitableDevice(VulkanStorage& storage)
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(storage.instance, &deviceCount, nullptr);
 
        ASSERT(deviceCount != 0, "Failed to find GPUs with Vulkan support.");
 
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(storage.instance, &deviceCount, devices.data());
 
        for (const auto& device : devices)
        {
            if (isDeviceSuitable(device, storage))
            {
                storage.physicalDevice = device;
                storage.msaaSamples = getMaxUsableSampleCount(storage);
                break;
            }
        }
 
        ASSERT(storage.physicalDevice != VK_NULL_HANDLE, "Failed to find a suitable GPU.");

    }
 
    bool VkDeviceBuilder::isDeviceSuitable(const VkPhysicalDevice& device, VulkanStorage& storage)
    {
        // Basic device properties like the name, type and supported Vulkan version.
        [[maybe_unused]] VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
 
        // The support for optional features like texture compression, 64-bit floats and multi viewport rendering.
        [[maybe_unused]] VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
 
        // Check if device can process the commands we want to use.
        const utils::QueueFamilyIndices queueFamilyIndices = utils::findQueueFamilies(device, storage.surface);
 
        // Check if physical device supports swap chain extension.
        const bool extensionsSupported = isDeviceExtensionSupported(device);
 
        // Check if physical device supports swap chain.
        const utils::SwapChainSupportDetails swapChainSupport = utils::querySwapChainSupport(device, storage.surface);
 
        // Check anisotropic filtering
        VkPhysicalDeviceFeatures supportedFeatures;
        vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
 
        return queueFamilyIndices.isComplete()
            && extensionsSupported
            && swapChainSupport.isComplete()
            && supportedFeatures.samplerAnisotropy;
    }
 
	bool VkDeviceBuilder::isDeviceExtensionSupported(const VkPhysicalDevice& device) 
    {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
 
        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
 
        std::unordered_set<std::string> requiredExtensionsSet(REQUIRED_DEVICE_EXTENSIONS.begin(), REQUIRED_DEVICE_EXTENSIONS.end());
 
        for (const auto& extension : availableExtensions)
        {
            requiredExtensionsSet.erase(extension.extensionName);
        }
 
        return requiredExtensionsSet.empty();
    }
 
    VkSampleCountFlagBits VkDeviceBuilder::getMaxUsableSampleCount(const VulkanStorage& storage)
    {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(storage.physicalDevice, &physicalDeviceProperties);
 
        const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
 
        return VK_SAMPLE_COUNT_1_BIT;
    }


}
