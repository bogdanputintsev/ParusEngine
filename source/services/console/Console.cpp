#include "Console.h"

namespace parus
{

    void Console::registerConsoleCommand(const std::string& command, CommandCallback callback)
    {
        commands[command] = std::move(callback);
        trie.insert(command);
    }

    std::string Console::processCommand(const std::string& command) const
    {
        const auto it = commands.find(command);
        if (it != commands.end())
        {
            return it->second();
        }
        return "Unknown command: '" + command + "'.";
    }

    std::string Console::hintNext(const std::string& input) const
    {
        return trie.hintNext(input);
    }

}
