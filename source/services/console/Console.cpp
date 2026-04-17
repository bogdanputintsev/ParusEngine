#include "Console.h"

#include <sstream>

namespace parus
{
    void Console::registerConsoleCommand(const std::string& command, CommandCallback callback)
    {
        commands[command] = std::move(callback);
        trie.insert(command);
    }

    std::string Console::processCommand(const std::string& command) const
    {
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
                std::vector<std::string> args(tokens.begin() + tokenCount, tokens.end());
                return it->second(args);
            }
        }

        return "Unknown command: '" + command + "'.";
    }

    std::string Console::hintNext(const std::string& input) const
    {
        return trie.hintNext(input);
    }
}
