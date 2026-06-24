#pragma once
#include <filesystem>
#include <optional>
#include <string>

#include "services/renderer/vulkan/mesh/Mesh.h"

namespace parus::serialization
{

    /**
     * Writes a single .pmesh file for the given mesh.
     * Returns the stem used as the filename (empty on failure).
     */
    std::string writeMesh(
        const parus::Mesh& mesh,
        const std::filesystem::path& outputDir);

    /** Reads a .pmesh file and lazily loads its textures from .ptex files. Returns nullopt on failure. */
    std::optional<parus::Mesh> readMesh(
        const std::string& stem,
        const std::filesystem::path& meshesDir,
        const std::filesystem::path& texturesDir);

}
