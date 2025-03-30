#pragma once
#include <filesystem>

#include "renderer/vulkan/mesh/Mesh.h"

namespace tessera
{
    
    class ObjImporter final
    {
    public:
        static Mesh importFromFile(const std::string& filePath); 
    };
    
}
