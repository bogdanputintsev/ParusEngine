#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "utils/TesseraLog.h"
#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class DebugManager final : public Initializable
	{
	public:
		void init() override;

		static void populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

		void clean() override;

		static void checkValidationLayerSupport();

		static bool validationLayersAreEnabled();

		static std::vector<const char*> getValidationLayers();

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);

		static LogType getLogType(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity);

		static VkResult createDebugUtilsMessengerExt(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
		                                             const VkAllocationCallbacks* pAllocator,
		                                             VkDebugUtilsMessengerEXT* pDebugMessenger);

		static void destroyDebugUtilsMessengerExt(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

		VkDebugUtilsMessengerEXT debugMessenger = nullptr;

	};

}


