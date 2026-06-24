#pragma once
#include <string>

#include "services/Service.h"

namespace parus
{

    class Serialization final : public Service
    {
    public:
        /** Registers the save and open console commands. */
        void registerConsoleCommands();

        /** Saves all current world meshes, textures, and scene state to binary format. */
        std::string saveCurrentWorld(const std::string& sceneName);

        /** Loads a previously saved scene by name, replacing the current world state. */
        std::string importWorld(const std::string& sceneName);
    };

}