#pragma once
#include <array>

#include "services/renderer/vulkan/mesh/Mesh.h"

namespace parus
{

class SkyboxMesh
{
public:
    /** Creates a unit cube mesh for skybox rendering. Camera is always inside. */
    [[nodiscard]] static Mesh create()
    {
        // Corners of the unit cube: index encodes sign bits (bit0=x, bit1=y, bit2=z)
        static constexpr std::array<math::Vector3, 8> POSITIONS = {{
            { -1.0f, -1.0f, -1.0f }, // 0
            {  1.0f, -1.0f, -1.0f }, // 1
            {  1.0f,  1.0f, -1.0f }, // 2
            { -1.0f,  1.0f, -1.0f }, // 3
            { -1.0f, -1.0f,  1.0f }, // 4
            {  1.0f, -1.0f,  1.0f }, // 5
            {  1.0f,  1.0f,  1.0f }, // 6
            { -1.0f,  1.0f,  1.0f }, // 7
        }};

        // CCW from outside (standard OBJ convention used by the original dynamic_skybox.obj)
        static constexpr std::array<uint32_t, 36> INDICES = {{
            0, 1, 2,  0, 2, 3, // -Z face
            5, 4, 7,  5, 7, 6, // +Z face
            4, 0, 3,  4, 3, 7, // -X face
            1, 5, 6,  1, 6, 2, // +X face
            4, 5, 1,  4, 1, 0, // -Y face
            3, 2, 6,  3, 6, 7, // +Y face
        }};

        std::vector<math::Vertex> vertices;
        vertices.reserve(POSITIONS.size());

        for (const auto& position : POSITIONS)
        {
            math::Vertex vertex{};
            vertex.position = position;
            vertices.push_back(vertex);
        }

        MeshPart part{};
        part.vertices = std::move(vertices);
        part.indices  = std::vector<uint32_t>(INDICES.begin(), INDICES.end());

        Mesh mesh{};
        mesh.meshType = MeshType::SKY;
        mesh.meshParts.push_back(std::move(part));

        return mesh;
    }
};

}
