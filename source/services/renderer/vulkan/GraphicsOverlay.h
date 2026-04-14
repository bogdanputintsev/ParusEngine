#pragma once
#include <vulkan/vulkan_core.h>

namespace parus::vulkan
{
    class GraphicsOverlay
    {
    public:
        virtual ~GraphicsOverlay() = default;
        virtual void render(VkCommandBuffer commandBuffer) = 0;
    };
}