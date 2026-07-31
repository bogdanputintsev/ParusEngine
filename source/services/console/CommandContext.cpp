#include "CommandContext.h"

#include "engine/Event.h"
#include "services/Services.h"

namespace parus
{
    CommandContext::CommandContext(std::string command)
        : command(std::move(command))
    {
    }

    void CommandContext::write(const std::string& message)
    {
        if (!aggregatedOutput.empty())
        {
            aggregatedOutput += '\n';
        }
        aggregatedOutput += message;

        FIRE_EVENT(EventType::EVENT_CONSOLE_COMMAND_PROGRESS, command, message);
    }

    std::string_view CommandContext::output() const
    {
        return aggregatedOutput;
    }
}
