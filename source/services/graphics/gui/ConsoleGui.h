#pragma once
#include <string>
#include <vector>

struct ImGuiInputTextCallbackData;
struct ImFont;

namespace parus::imgui
{
    enum class ConsoleEntryType
    {
        INFO,
        COMMAND,
        REPLY
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
        void registerConsoleCommands();
        void draw();
        void setFont(ImFont* font);
        [[nodiscard]] bool isFocused() const;

    private:
        void onNewCommandSent();
        void moveHistoryUp(std::string& text);
        void moveHistoryDown(std::string& text);
        static int inputTextCallback(ImGuiInputTextCallbackData* data);

        static constexpr int MAX_COMMAND_HISTORY_SIZE = 10;

        std::string commandLineText;
        std::vector<ConsoleEntry> historyEntries{ {.type = ConsoleEntryType::INFO, .text = "Parus Engine" } };
        bool scrollToBottom{ false };
        bool scrollToTop{ true };
        ImFont* consoleFont{ nullptr };
        bool isVisible{ false };
        bool windowFocused{ false };
        bool inputTextActive{ false };
        bool focusInputNextFrame{ false };

        std::vector<std::string> commandHistory;
        int historyIndex{ -1 };
        std::string savedCurrentInput;
    };
}
