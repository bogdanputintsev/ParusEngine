#pragma once
#include <expected>
#include <filesystem>
#include <string>
#include <unordered_map>

#include "ConfigError.h"
#include "services/Service.h"


namespace parus
{
    /** Reads and writes INI-style config files, grouped by section. */
    class Configs final : public Service
    {
    public:
        void loadAll();

        /** Reads a config value as type T. Asserts if it is missing or invalid. */
        template <typename T>
        T get(const std::string& group, const std::string& key);

        /** Reads a config value as type T, falling back to defaultValue if it is missing or invalid. */
        template <typename T>
        T getOrDefault(const std::string& group, const std::string& key, T defaultValue);

        void write(const std::string& group, const std::string& key, const std::string& newValue);

        std::unordered_map<std::string, std::string> getByGroup(const std::string& group);

    private:
        static constexpr auto CONFIG_FOLDER = "config";
        static constexpr auto CONFIG_FILE_EXTENSION = ".ini";

        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> configData;

        void loadConfigFile(const std::filesystem::path& configFilename);

        std::expected<std::string, ConfigError> getRawValue(const std::string& group, const std::string& key);
    };
}
