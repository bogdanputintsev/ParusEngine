#include "ConsoleGui.h"

#pragma warning(push, 0)
#include "third-party/imgui/imgui.h"
#pragma warning(pop)

#include "services/graphics/imgui/Theme.h"

#include "engine/Event.h"
#include "engine/input/Input.h"
#include "engine/logs/Logs.h"
#include "services/Services.h"
#include "services/console/Console.h"

#include <sstream>

namespace parus::imgui
{
    ConsoleGui::ConsoleGui()
    {
        commandLineText.reserve(256);
    }

    void ConsoleGui::registerEvents()
    {
        REGISTER_EVENT(EventType::EVENT_KEY_PRESSED, [&](const KeyButton key)
        {
           if (key == KeyButton::KEY_GRAVE || key == KeyButton::KEY_X)
           {
               isVisible = !isVisible;
               if (isVisible)
               {
                   scrollToTop = true;
               }
           }
        });
    }

    void ConsoleGui::draw()
    {
        if (!isVisible)
        {
            return;
        }

        if (consoleFont)
        {
            ImGui::PushFont(consoleFont);
        }

        ImGui::PushStyleColor(ImGuiCol_WindowBg,      theme::COLOR_BACKGROUND);
        ImGui::PushStyleColor(ImGuiCol_TitleBg,        theme::COLOR_TITLEBAR);
        ImGui::PushStyleColor(ImGuiCol_TitleBgActive,  theme::COLOR_TITLEBAR);
        ImGui::PushStyleColor(ImGuiCol_Border,         theme::COLOR_BORDER);
        ImGui::PushStyleColor(ImGuiCol_FrameBg,        theme::COLOR_TITLEBAR);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding,  4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,  ImVec2(10.0f, 8.0f));
        ImGui::SetNextWindowBgAlpha(0.92f);

        ImGui::Begin("Console", nullptr);
        {
            const float historyHeight = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing();

            ImGui::BeginChild("##history", ImVec2(-FLT_MIN, historyHeight), false,
                ImGuiWindowFlags_None);
            {
                for (const ConsoleEntry& entry : historyEntries)
                {
                    switch (entry.type)
                    {
                        case ConsoleEntryType::INFO:
                        {
                            ImGui::TextColored(theme::COLOR_INFO, "%s", entry.text.c_str());
                            break;
                        }
                        case ConsoleEntryType::COMMAND:
                        {
                            ImGui::TextColored(theme::COLOR_COMMAND, "%s", entry.text.c_str());
                            break;
                        }
                        case ConsoleEntryType::REPLY:
                        {
                            const bool isIndented = !entry.text.empty()
                                && (entry.text[0] == ' ' || entry.text[0] == '\t');
                            if (isIndented)
                            {
                                ImGui::TextColored(theme::COLOR_REPLY_VALUE, "%s", entry.text.c_str());
                            }
                            else
                            {
                                ImGui::TextColored(theme::COLOR_REPLY_TEXT, "%s", entry.text.c_str());
                            }
                            break;
                        }
                        default:
                        {
                            break;
                        }
                    }
                }

                if (scrollToBottom)
                {
                    ImGui::SetScrollHereY(1.0f);
                    scrollToBottom = false;
                }
                else if (scrollToTop)
                {
                    ImGui::SetScrollY(0.0f);
                    scrollToTop = false;
                }
            }
            ImGui::EndChild();

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
        }
        ImGui::End();

        ImGui::PopStyleVar(3);
        ImGui::PopStyleColor(5);

        if (consoleFont)
        {
            ImGui::PopFont();
        }
    }

    void ConsoleGui::onNewCommandSent()
    {
        if (!commandLineText.empty())
        {
            historyEntries.push_back({ ConsoleEntryType::COMMAND, "> " + commandLineText });
            LOG_INFO("[Console] > " + commandLineText);

            std::string output = Services::get<Console>()->processCommand(commandLineText);
            std::istringstream stream(output);
            std::string line;
            while (std::getline(stream, line))
            {
                if (!line.empty())
                {
                    historyEntries.push_back({ ConsoleEntryType::REPLY, line });
                    LOG_INFO("[Console] " + line);
                }
            }

            commandLineText.clear();
            scrollToBottom = true;
        }
    }

    void ConsoleGui::registerConsoleCommands()
    {
        Services::get<Console>()->registerConsoleCommand("clear", [this](const auto&)
        {
            historyEntries.clear();
            scrollToTop = true;
            return std::string();
        });
    }

    void ConsoleGui::setFont(ImFont* font)
    {
        consoleFont = font;
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
