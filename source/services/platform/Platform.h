#pragma once

#include <vulkan/vulkan_core.h>

#include "engine/Defines.h"
#include "services/Service.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"

#ifdef WITH_WINDOWS_PLATFORM
#define NOMINMAX
#include <windows.h>
#endif

namespace parus::imgui
{
    class ImGuiLibrary;
}

namespace parus
{
    
    class Platform final : public Service
    {
    public:
        void init();
        void clean();

        static void getMessages();
        void processOnResize() const;
        void createVulkanSurface(vulkan::VulkanStorage& vulkanStorage) const;
        
        friend class parus::imgui::ImGuiLibrary;
    private:
#ifdef WITH_WINDOWS_PLATFORM
        struct PlatformState
        {
        public:
            HINSTANCE hinstance;
            HWND hwnd;
            float clockFrequency;
            LARGE_INTEGER startTime;

            friend class parus::imgui::ImGuiLibrary;
        };
#endif
        PlatformState platformState;
        
    };
    
}


