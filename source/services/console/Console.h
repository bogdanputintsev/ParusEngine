#pragma once

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "services/Service.h"
#include "services/console/CommandContext.h"
#include "services/console/Trie.h"

namespace parus
{

    class Console final : public Service
    {
    public:
        using CommandCallback = std::function<void(const std::vector<std::string>&, CommandContext&)>;
        /** Receives the full input line; returns a completed line, or nullopt to defer to the next provider / the Trie. */
        using CompletionProvider = std::function<std::optional<std::string>(const std::string& input)>;

        void registerConsoleCommand(const std::string& command, CommandCallback callback);
        std::string submitCommand(const std::string& command);
        /** Providers are tried in registration order first; the static Trie is the fallback. */
        void registerCompletionProvider(CompletionProvider provider);
        [[nodiscard]] std::string hintNext(const std::string& input) const;

    private:
        std::unordered_map<std::string, CommandCallback> commands;
        Trie trie;
        std::vector<CompletionProvider> completionProviders;
    };

}
