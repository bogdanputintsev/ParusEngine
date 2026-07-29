#pragma once
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "services/console/reflection/PropertyRegistry.h"
#include "services/world/entity/Entity.h"

namespace parus
{
    class Console;
    class CommandContext;
    class EntityManager;
    class SpectatorCamera;

    /** Wires the generic get/set/list console commands onto entity and component property schemas. */
    class ConsoleReflection final
    {
    public:
        ConsoleReflection(std::shared_ptr<Console> console, std::shared_ptr<EntityManager> entityManager, SpectatorCamera* camera = nullptr);

        /** Parses a target token: "#5" -> id 5, otherwise a name lookup. Nullopt if unresolved. */
        [[nodiscard]] std::optional<EntityId> resolveEntityId(const std::string& targetToken) const;

    private:
        void registerCommands();

        /** Registers every target schema (entity intrinsics and each component type). */
        void buildEntitySchemas();
        void buildEntityIntrinsicsSchema();
        void buildPointLightSchema();
        void buildDirectionalLightSchema();
        void buildSkyboxSchema();
        void buildMeshSchema();

        /** Splits "Door.PointLight.intensity" into { "Door", "PointLight", "intensity" }. */
        static std::vector<std::string> splitAddress(const std::string& address);

        /**
         * Completion provider registered with Console: completes the dotted address after
         * get/set/list. Returns nullopt for anything else, deferring to the static Trie.
         */
        [[nodiscard]] std::optional<std::string> completeAddress(const std::string& input) const;

        void handleGet(const std::vector<std::string>& args, CommandContext& out) const;
        void handleSet(const std::vector<std::string>& args, CommandContext& out) const;
        void handleList(const std::vector<std::string>& args, CommandContext& out) const;

        std::shared_ptr<Console> console;
        std::shared_ptr<EntityManager> entityManager;
        /** Optional bridge to the world's camera, addressed via the special "camera" target name. */
        SpectatorCamera* camera = nullptr;
        PropertyRegistry registry;
    };

}
