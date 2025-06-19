#include "Configs.h"

#include <filesystem>

#include "engine/EngineCore.h"
#include "engine/utils/Utils.h"

namespace parus
{
    void Configs::loadAll()
    {
        ASSERT(std::filesystem::exists(CONFIG_FOLDER) && std::filesystem::is_directory(CONFIG_FOLDER),
            "Config folder must exist.");

        for (const auto& configFile : std::filesystem::directory_iterator(CONFIG_FOLDER))
        {
            if (std::filesystem::is_regular_file(configFile) && configFile.path().extension() == CONFIG_FILE_EXTENSION)
            {
                loadConfigFile(configFile.path().string());
            }
        }
    }

    std::string Configs::get(const std::string& group, const std::string& key, const std::string& defaultValue)
    {
        const auto& groupPosition = configData.find(group);
        if (groupPosition != configData.end())
        {
            const auto keyPosition = groupPosition->second.find(key);
            if (keyPosition != groupPosition->second.end())
            {
                return keyPosition->second;
            }
        }
        
        return defaultValue;
    }

    void Configs::write(const std::string& group, const std::string& key, const std::string& newValue)
    {
        configData[group][key] = newValue;
    }

    std::unordered_map<std::string, std::string> Configs::getByGroup(const std::string& group)
    {
        return configData[group];
    }

    std::optional<bool> Configs::getAsBool(const std::string& group, const std::string& key)
    {
        const std::string result = get(group, key);
        if (result == "true" || result == "false")
        {
            return result == "true";
        }

        return std::nullopt;
    }

    std::optional<int> Configs::getAsInt(const std::string& group, const std::string& key)
    {
        const std::string resultString = get(group, key);
        
        try
        {
            return std::stoi(resultString);
        }
        catch ([[maybe_unused]] std::exception& exception)
        {
            return std::nullopt;
        }
    }

    void Configs::loadConfigFile(const std::filesystem::path& configFilename)
    {
        utils::readFile(configFilename.string(), [&](std::ifstream& file)
        {
            std::string line;
            std::string currentGroup;

            while (std::getline(file, line))
            {
                line = utils::string::trim(line);
                if (line.empty() || line[0] == ';' || line[0] == '#')
                {
                    continue;
                }

                if (line.front() == '[' && line.back() == ']')
                {
                    // Group section, e.g. [engine]
                    currentGroup = line.substr(1, line.size() - 2);
                    currentGroup = utils::string::trim(currentGroup);
                }
                else
                {
                    // Key-values section
                    const auto delimiterPosition = line.find('=');
                    if (delimiterPosition == std::string::npos)
                    {
                        continue;
                    }

                    std::string key = line.substr(0, delimiterPosition);
                    std::string value = line.substr(delimiterPosition + 1);

                    key = utils::string::trim(key);
                    value = utils::string::trim(value);

                    if (currentGroup.empty())
                    {
                        currentGroup = "common";
                    }
                    
                    configData[currentGroup][key] = value;
                }
            }
        });
    }
}

