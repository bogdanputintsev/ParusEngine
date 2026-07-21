#pragma once

namespace parus
{
    /** Reasons a config lookup or parse can fail. */
    enum class ConfigError
    {
        GroupNotFound,
        KeyNotFound,
        ParseFailure,
    };
}
