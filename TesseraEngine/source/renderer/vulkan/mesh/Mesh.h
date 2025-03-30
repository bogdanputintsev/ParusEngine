#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>


namespace tessera::math
{
    struct Vertex;
}

namespace tessera
{
    struct Texture;

    struct MeshPart
    {
        size_t vertexOffset;
        size_t vertexCount;
        size_t indexOffset;
        size_t indexCount;
        std::shared_ptr<Texture> texture;
        std::vector<VkDescriptorSet> descriptorSets;

        std::vector<tessera::math::Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    struct Mesh
    {
        std::vector<MeshPart> meshParts;
    };
    
}
