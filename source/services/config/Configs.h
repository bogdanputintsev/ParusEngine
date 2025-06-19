#pragma once
#include <filesystem>
#include <string>
#include <unordered_map>

#include "services/Service.h"


namespace parus
{
    
    class Configs final : public Service
    {
    public:
        void loadAll();
        
        std::string get(const std::string& group, const std::string& key, const std::string& defaultValue = "");
        void write(const std::string& group, const std::string& key, const std::string& newValue);
        
        std::unordered_map<std::string, std::string> getByGroup(const std::string& group);
        
        std::optional<bool> getAsBool(const std::string& group, const std::string& key);
        std::optional<int> getAsInt(const std::string& group, const std::string& key);
    private:
        static constexpr auto CONFIG_FOLDER = "config";
        static constexpr auto CONFIG_FILE_EXTENSION = ".ini";

        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> configData;

        void loadConfigFile(const std::filesystem::path& configFilename);
    };
}
