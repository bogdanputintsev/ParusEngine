#pragma once

#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace parus
{

    struct TrieNode
    {
        std::map<std::string, std::unique_ptr<TrieNode>> children;
        bool isTerminal = false;
    };

    class Trie
    {
    public:
        void insert(const std::string& command)
        {
            const std::vector<std::string> words = splitWords(command);
            TrieNode* current = &root;

            for (const auto& word : words)
            {
                if (!current->children.contains(word))
                {
                    current->children[word] = std::make_unique<TrieNode>();
                }
                current = current->children[word].get();
            }

            current->isTerminal = true;
        }

        std::string hintNext(const std::string& input) const
        {
            if (input.empty())
            {
                if (!root.children.empty())
                {
                    return root.children.begin()->first;
                }
                return "";
            }

            const std::vector<std::string> words = splitWords(input);
            const bool hasTrailingSpace = (input.back() == ' ');

            if (words.empty())
            {
                if (!root.children.empty())
                {
                    return root.children.begin()->first;
                }
                return "";
            }

            if (hasTrailingSpace)
            {
                return completeNextWord(words);
            }
            
            return cycleCurrentWord(words);
        }

    private:
        TrieNode root;

        std::string completeNextWord(const std::vector<std::string>& words) const
        {
            const TrieNode* current = &root;

            for (const auto& word : words)
            {
                auto it = current->children.find(word);
                if (it == current->children.end())
                {
                    return joinWords(words);
                }
                current = it->second.get();
            }

            if (current->children.empty())
            {
                return joinWords(words);
            }

            std::string result = joinWords(words) + " " + current->children.begin()->first;
            return result;
        }

        std::string cycleCurrentWord(const std::vector<std::string>& words) const
        {
            const TrieNode* parentNode = &root;

            for (size_t i = 0; i < words.size() - 1; i++)
            {
                auto it = parentNode->children.find(words[i]);
                if (it == parentNode->children.end())
                {
                    return joinWords(words);
                }
                parentNode = it->second.get();
            }

            const std::string& currentWord = words.back();
            const auto& siblings = parentNode->children;

            const auto exactMatch = siblings.find(currentWord);
            if (exactMatch != siblings.end())
            {
                auto next = std::next(exactMatch);
                if (next == siblings.end())
                {
                    next = siblings.begin();
                }

                std::string result = joinWords(std::vector<std::string>(words.begin(), words.end() - 1));
                if (!result.empty())
                {
                    result += " ";
                }
                result += next->first;
                return result;
            }

            const auto prefixMatch = siblings.lower_bound(currentWord);
            if (prefixMatch != siblings.end() && prefixMatch->first.starts_with(currentWord))
            {
                std::string result = joinWords(std::vector<std::string>(words.begin(), words.end() - 1));
                if (!result.empty())
                {
                    result += " ";
                }
                result += prefixMatch->first;
                return result;
            }

            return joinWords(words);
        }

        static std::vector<std::string> splitWords(const std::string& input)
        {
            std::vector<std::string> words;
            std::istringstream stream(input);
            std::string word;

            while (stream >> word)
            {
                words.push_back(word);
            }

            return words;
        }

        static std::string joinWords(const std::vector<std::string>& words)
        {
            std::string result;

            for (size_t i = 0; i < words.size(); i++)
            {
                if (i > 0)
                {
                    result += " ";
                }
                result += words[i];
            }

            return result;
        }
    };

}
