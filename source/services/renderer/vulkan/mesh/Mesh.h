#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <filesystem>

#include "engine/utils/math/Math.h"
#include "services/renderer/Material.h"

namespace parus
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
        std::shared_ptr<parus::Material> material;
        
        std::vector<math::Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Mesh
    {
        MeshType meshType;
        std::vector<MeshPart> meshParts;
        /** Path to the source asset file. Empty when the mesh has no backing file (e.g. procedural geometry). */
        std::optional<std::string> sourcePath;
    };

    Mesh importMeshFromFile(const std::string& filePath);
    
}
