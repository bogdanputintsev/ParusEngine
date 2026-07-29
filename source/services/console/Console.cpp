#include "Console.h"

#include <sstream>

#include "engine/Event.h"
#include "services/Services.h"

namespace parus
{
    void Console::registerConsoleCommand(const std::string& command, CommandCallback callback)
    {
        commands[command] = std::move(callback);
        trie.insert(command);
    }

    std::string Console::submitCommand(const std::string& command)
    {
        FIRE_EVENT(EventType::EVENT_CONSOLE_COMMAND_SUBMITTED, command);

        std::vector<std::string> tokens;
        std::istringstream tokenStream(command);
        std::string token;
        while (tokenStream >> token)
        {
            tokens.push_back(token);
        }

        for (int tokenCount = static_cast<int>(tokens.size()); tokenCount >= 1; --tokenCount)
        {
            std::string commandName;
            for (int i = 0; i < tokenCount; ++i)
            {
                if (i > 0)
                {
                    commandName += ' ';
                }
                commandName += tokens[i];
            }

            const auto it = commands.find(commandName);
            if (it != commands.end())
            {
                [[maybe_unused]] const auto& [foundCommandName, commandCallback] = *it;

                const std::vector<std::string> args(tokens.begin() + tokenCount, tokens.end());
                CommandContext context(command);
                commandCallback(args, context);

                const std::string result(context.output());
                FIRE_EVENT(EventType::EVENT_CONSOLE_COMMAND_FINISHED, command, result);

                return result;
            }
        }

        const std::string result = "Unknown command: '" + command + "'.";
        FIRE_EVENT(EventType::EVENT_CONSOLE_COMMAND_FINISHED, command, result);

        return result;
    }

    void Console::registerCompletionProvider(CompletionProvider provider)
    {
        completionProviders.push_back(std::move(provider));
    }

    std::string Console::hintNext(const std::string& input) const
    {
        for (const CompletionProvider& provider : completionProviders)
        {
            if (const std::optional<std::string> hint = provider(input))
            {
                return *hint;
            }
        }

        return trie.hintNext(input);
    }
}
