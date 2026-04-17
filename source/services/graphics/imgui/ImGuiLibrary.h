#pragma once

#include <vulkan/vulkan_core.h>

#pragma warning(push, 0)
#include "third-party/imgui/imgui.h"
#pragma warning(pop)

#include "services/graphics/GraphicsLibrary.h"
#include "services/graphics/gui/ConsoleGui.h"
#include "services/renderer/vulkan/GraphicsOverlay.h"


namespace parus
{
    enum class KeyButton : uint8_t;
}


namespace parus::imgui
{
    class ImGuiLibrary final : public GraphicsLibrary, public parus::vulkan::GraphicsOverlay
    {
    public:
        void init() override;
        void drawFrame() override;
        void draw();
        void render(VkCommandBuffer commandBuffer) override;
        void handleMinimization() override;
        [[nodiscard]] std::vector<const char*> getRequiredExtensions() const override;
        void clean() override;
        [[nodiscard]] bool isCapturingInput() const override;
    private:
        static void renderDrawData(VkCommandBuffer cmd);
        static ImGuiKey getImGuiKeyCode(const ::parus::KeyButton keyTest);

        ConsoleGui consoleGui{};
    };


}
