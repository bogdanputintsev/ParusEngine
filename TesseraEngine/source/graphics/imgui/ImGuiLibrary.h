#pragma once

#include <vulkan/vulkan_core.h>

#include "imgui.h"
#include "graphics/GraphicsLibrary.h"
#include "graphics/gui/ConsoleGui.h"


namespace tessera
{
    enum class KeyButton : uint8_t;
}


namespace tessera::imgui
{
    class ImGuiLibrary final : public GraphicsLibrary
    {
    public:
        void init() override;
        void drawFrame() override;
        void draw();
        static void renderDrawData(VkCommandBuffer cmd);
        void handleMinimization() override;
        [[nodiscard]] std::vector<const char*> getRequiredExtensions() const override;
        void clean() override;
    private:
        static ImGuiKey getImGuiKeyCode(const ::tessera::KeyButton keyTest);

        ConsoleGui consoleGui{};
    };


}
