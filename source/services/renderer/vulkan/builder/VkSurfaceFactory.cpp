#include "VkSurfaceFactory.h"

#include "engine/Defines.h"
#include "engine/EngineCore.h"
#include "services/platform/Platform.h"

#ifdef WITH_WINDOWS_PLATFORM
#define NOMINMAX
#include <windows.h>
#endif

#include <vulkan/vulkan_win32.h>

namespace parus::vulkan
{

    void VkSurfaceFactory::build(VulkanStorage& vulkanStorage)
    {
        const auto platform = Services::get<Platform>();

#ifdef WITH_WINDOWS_PLATFORM
        const VkWin32SurfaceCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hinstance = static_cast<HINSTANCE>(platform->getInstanceHandle()),
            .hwnd = static_cast<HWND>(platform->getWindowHandle()),
        };
        
        ASSERT(vkCreateWin32SurfaceKHR(vulkanStorage.instance, &createInfo, nullptr, &vulkanStorage.surface) == VK_SUCCESS,
               "Vulkan surface creation failed");
#endif
        
    }
}
