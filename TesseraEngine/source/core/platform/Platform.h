#pragma once

#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera
{
    
    struct PlatformState
    {
        virtual ~PlatformState() = default;
    };

    class Platform
    {
    public:
        void init(const char* applicationName, int posX, int posY, int width, int height);
        void clean();

        static void getMessages();
        VkSurfaceKHR createVulkanSurface(const VkInstance instance);
    private:
        std::unique_ptr<PlatformState> platformState;
    };
    
}


