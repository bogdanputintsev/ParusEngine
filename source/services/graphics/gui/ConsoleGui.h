#pragma once
#include <string>

struct ImGuiInputTextCallbackData;

namespace parus::imgui
{

    class ConsoleGui final
    {
    public:
        void registerEvents();
        void draw();

    private:
        void onNewCommandSent();
        static int inputTextCallback(ImGuiInputTextCallbackData* data);

        std::string commandLineText;
        std::string consoleHistory = "Parus Engine v0.3.0\n";

        bool isVisible{false};
    };
}
