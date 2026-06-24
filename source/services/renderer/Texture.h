#pragma once
#include <optional>
#include <string>

namespace parus
{
    class Texture
    {
    public:
        virtual ~Texture() = default;

        /** Path to the source asset file. Empty when the texture was created from raw pixel data. */
        std::optional<std::string> sourcePath;
    };
}