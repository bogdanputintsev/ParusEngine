#include "ConsoleGui.h"

#pragma warning(push, 0)
#include "third-party/imgui/imgui.h"
#include "third-party/imgui/imgui_internal.h"
#pragma warning(pop)

#include "engine/Event.h"
#include "engine/input/Input.h"
#include "services/Services.h"
#include "services/console/Console.h"

namespace parus::imgui
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
            ImGui::SetNextWindowBgAlpha(0.1f);
            ImGui::Begin("Console", nullptr);
            {
                commandLineText.reserve(50);
                consoleHistory.reserve(1024);

                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::IsWindowAppearing())
                {
                    ImGui::SetKeyboardFocusHere();
                }
                if (ImGui::InputText("##command", commandLineText.data(), commandLineText.capacity() + 1,
                    ImGuiInputTextFlags_CallbackCompletion, &ConsoleGui::inputTextCallback, this))
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
            consoleHistory += Services::get<Console>()->processCommand(commandLineText) + "\n";
            consoleHistory.resize(strlen(consoleHistory.data()));
            commandLineText = "";
        }
    }

    int ConsoleGui::inputTextCallback(ImGuiInputTextCallbackData* data)
    {
        if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion)
        {
            std::string currentInput(data->Buf, data->BufTextLen);
            std::string hint = Services::get<Console>()->hintNext(currentInput);

            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, hint.c_str());
        }
        return 0;
    }
}
