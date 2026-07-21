#include "Configs.h"

#include <filesystem>
#include <typeinfo>

#include "ConfigParser.h"
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

    template <typename T>
    T Configs::get(const std::string& group, const std::string& key)
    {
        const std::expected<T, ConfigError> result = getRawValue(group, key).and_then(ConfigParser<T>::parse);
        ASSERT(result.has_value(),
            "Failed to read config value [" + group + "] " + key + " as " + typeid(T).name());

        return result.value();
    }

    template <typename T>
    T Configs::getOrDefault(const std::string& group, const std::string& key, T defaultValue)
    {
        return getRawValue(group, key).and_then(ConfigParser<T>::parse).value_or(std::move(defaultValue));
    }

    template bool Configs::get<bool>(const std::string& group, const std::string& key);
    template int Configs::get<int>(const std::string& group, const std::string& key);
    template std::string Configs::get<std::string>(const std::string& group, const std::string& key);

    template bool Configs::getOrDefault<bool>(const std::string& group, const std::string& key, bool defaultValue);
    template int Configs::getOrDefault<int>(const std::string& group, const std::string& key, int defaultValue);
    template std::string Configs::getOrDefault<std::string>(const std::string& group, const std::string& key, std::string defaultValue);

    void Configs::write(const std::string& group, const std::string& key, const std::string& newValue)
    {
        configData[group][key] = newValue;
    }

    std::unordered_map<std::string, std::string> Configs::getByGroup(const std::string& group)
    {
        return configData[group];
    }

    std::expected<std::string, ConfigError> Configs::getRawValue(const std::string& group, const std::string& key)
    {
        const auto& groupPosition = configData.find(group);
        if (groupPosition == configData.end())
        {
            return std::unexpected(ConfigError::GroupNotFound);
        }

        const auto keyPosition = groupPosition->second.find(key);
        if (keyPosition == groupPosition->second.end())
        {
            return std::unexpected(ConfigError::KeyNotFound);
        }

        return keyPosition->second;
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
