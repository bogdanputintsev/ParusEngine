#pragma once
#include <filesystem>
#include <optional>
#include <string>

#include "SceneData.h"
#include "services/world/World.h"

namespace parus::serialization
{

    /**
     * Writes a .pworld file capturing all scene state from the given World.
     * All .pmesh stems in meshInstances must already be written to disk
     * before this is called.
     */
    void writeWorld(
        const parus::World& world,
        const std::string& sceneName,
        const std::filesystem::path& outputPath);

    /** Reads a .pworld file into a SceneData POD. Returns nullopt on failure. */
    std::optional<SceneData> readWorld(
        const std::string& sceneName,
        const std::filesystem::path& scenesDir);

}
