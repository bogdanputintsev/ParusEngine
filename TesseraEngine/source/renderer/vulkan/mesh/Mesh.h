#pragma once
#include <memory>
#include <vector>
#include <filesystem>

#include "math/Math.h"
#include "renderer/vulkan/material/Material.h"

namespace tessera
{

    enum class MeshType : uint8_t
    {
        STATIC_MESH,
        SKY
    };
    
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
        MeshType meshType;
        std::vector<MeshPart> meshParts;
    };

    Mesh importMeshFromFile(const std::string& filePath);
    
}
