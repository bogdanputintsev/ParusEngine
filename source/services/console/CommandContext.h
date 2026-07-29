#pragma once
#include <string>
#include <string_view>

namespace parus
{

    /** Output sink for one console command run: accumulates lines and emits a PROGRESS event per line. */
    class CommandContext final
    {
    public:
        explicit CommandContext(std::string command);

        /** Emits one output line: appends to the aggregate and fires EVENT_CONSOLE_COMMAND_PROGRESS. */
        void write(const std::string& message);

        /** Everything written so far, newline-separated. */
        [[nodiscard]] std::string_view output() const;

    private:
        std::string command;
        std::string aggregatedOutput;
    };

}
