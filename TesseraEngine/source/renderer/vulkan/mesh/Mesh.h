#pragma once
#include <memory>
#include <vector>
#include <filesystem>
#include <vulkan/vulkan_core.h>

#include "math/Math.h"
#include "renderer/vulkan/material/Material.h"

namespace tessera
{

    struct MeshPart
    {
        size_t vertexOffset;
        size_t vertexCount;
        size_t indexOffset;
        size_t indexCount;
        std::shared_ptr<vulkan::Material> material;
        
        std::vector<math::Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Mesh
    {
        std::vector<MeshPart> meshParts;
    };

    Mesh importMeshFromFile(const std::string& filePath);
    
}
