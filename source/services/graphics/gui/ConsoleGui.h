#pragma once
#include <string>
#include <vector>

struct ImGuiInputTextCallbackData;
struct ImFont;

namespace parus::imgui
{
    enum class ConsoleEntryType
    {
        Info,
        Command,
        Reply
    };

    struct ConsoleEntry
    {
        ConsoleEntryType type;
        std::string text;
    };

    class ConsoleGui final
    {
    public:
        ConsoleGui();
        void registerEvents();
        void draw();
        void setFont(ImFont* font);

    private:
        void onNewCommandSent();
        static int inputTextCallback(ImGuiInputTextCallbackData* data);

        std::string commandLineText;
        std::vector<ConsoleEntry> historyEntries{ { ConsoleEntryType::Info, "Parus Engine v0.3.0" } };
        bool scrollToBottom{ false };
        bool scrollToTop{ true };
        ImFont* consoleFont{ nullptr };

        bool isVisible{ false };
    };
}