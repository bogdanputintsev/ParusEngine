#pragma once
#include <renderer/vulkan/texture/Texture.h>

#include <filesystem>

namespace tessera
{
    
    class TextureImporter
    {
    public:
        static Texture importFromFile(const std::string& filePath);     
    };
    
}
