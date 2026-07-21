#pragma once
#include <expected>
#include <string>

#include "ConfigError.h"

namespace parus
{
    /**
     * Converts a raw config string into type T.
     * Add a specialization here to support a new config value type.
     */
    template <typename T>
    struct ConfigParser;

    template <>
    struct ConfigParser<std::string> final
    {
        static std::expected<std::string, ConfigError> parse(const std::string& rawValue)
        {
            return rawValue;
        }
    };

    template <>
    struct ConfigParser<bool> final
    {
        static std::expected<bool, ConfigError> parse(const std::string& rawValue)
        {
            if (rawValue == "true")
            {
                return true;
            }

            if (rawValue == "false")
            {
                return false;
            }

            return std::unexpected(ConfigError::ParseFailure);
        }
    };

    template <>
    struct ConfigParser<int> final
    {
        static std::expected<int, ConfigError> parse(const std::string& rawValue)
        {
            try
            {
                return std::stoi(rawValue);
            }
            catch ([[maybe_unused]] const std::exception& exception)
            {
                return std::unexpected(ConfigError::ParseFailure);
            }
        }
    };
}
