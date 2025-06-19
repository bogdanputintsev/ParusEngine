#pragma once

#include <vulkan/vulkan_core.h>

#include "core/Defines.h"

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
    
    class Platform
    {
    public:
        void init();
        void clean();

        static void getMessages();
        void processOnResize();
        [[nodiscard]] VkSurfaceKHR createVulkanSurface(const VkInstance& instance) const;

        struct WindowInfo
        {
            const char* title = "Tessera Engine";
            int positionX = 250;
            int positionY = 250;
            int width = 800;
            int height = 600;
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


