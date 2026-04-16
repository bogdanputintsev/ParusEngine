#pragma once

#include <functional>
#include <string>
#include <unordered_map>

#include "services/Service.h"
#include "services/console/Trie.h"

namespace parus
{

    class Console final : public Service
    {
    public:
        using CommandCallback = std::function<std::string()>;

        void registerConsoleCommand(const std::string& command, CommandCallback callback);
        std::string processCommand(const std::string& command) const;
        std::string hintNext(const std::string& input) const;

    private:
        std::unordered_map<std::string, CommandCallback> commands;
        Trie trie;
    };

}
