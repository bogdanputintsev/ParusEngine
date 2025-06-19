#include "ConsoleGui.h"

#include "Event.h"
#include "imgui.h"
#include "core/Input.h"

namespace tessera::imgui
{
    void ConsoleGui::registerEvents()
    {
        REGISTER_EVENT(EventType::EVENT_KEY_PRESSED, [&](const KeyButton key)
        {
           if (key == KeyButton::KEY_GRAVE)
           {
               isVisible = !isVisible;
           }
        });
    }

    void ConsoleGui::draw()
    {
        if (isVisible)
        {
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(250, ImGui::GetTextLineHeight() * 19));
            ImGui::SetNextWindowBgAlpha(0.1f);
            ImGui::Begin("Console", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
            {
                ImGui::SetNextItemWidth(-FLT_MIN); 
                if (ImGui::InputText("##command", commandLineText.data(), commandLineText.capacity() + 1))
                {
                    commandLineText.resize(strlen(commandLineText.data()));
                }
                
                if (ImGui::IsItemDeactivatedAfterEdit())
                {
                    onNewCommandSent();
                }
                
                ImGui::InputTextMultiline("##console", consoleHistory.data(), consoleHistory.capacity() + 1,
                    ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16),
                    ImGuiInputTextFlags_ReadOnly);
            }
            ImGui::End();
        }
    }

    void ConsoleGui::onNewCommandSent()
    {
        if (!commandLineText.empty())
        {
            consoleHistory += "> " + commandLineText + "\n";
            commandLineText = "";
        }
    }
}
