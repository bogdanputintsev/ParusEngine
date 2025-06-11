#pragma once

#include <vulkan/vulkan_core.h>

#include "core/Defines.h"
#include "services/Service.h"

#ifdef WITH_WINDOWS_PLATFORM
#define NOMINMAX
#include <windows.h>
#endif

namespace tessera::imgui
{
    class ImGuiLibrary;
}

namespace tessera
{
    
    class Platform final : public Service
    {
    public:
        void init();
        void clean();

        void getMessages();
        void processOnResize();
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(const VkInstance& instance) const;

        
        struct WindowInfo
        {
            const char* title = "Tessera Engine";
            int positionX = 250;
            int positionY = 250;
            int width = 1200;
            int height = 900;
            bool isMinimized = false;
        };

        [[nodiscard]] WindowInfo getWindowInfo() const { return windowInfo; }
        
        friend class tessera::imgui::ImGuiLibrary;
        
    private:
        WindowInfo windowInfo;
        
#ifdef WITH_WINDOWS_PLATFORM
        struct PlatformState
        {
        public:
            HINSTANCE hinstance;
            HWND hwnd;
            float clockFrequency;
            LARGE_INTEGER startTime;

            friend class tessera::imgui::ImGuiLibrary;
        };
#endif
        PlatformState platformState;
        
    };
    
}


