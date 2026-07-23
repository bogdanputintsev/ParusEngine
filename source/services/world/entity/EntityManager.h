#pragma once
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Components.h"
#include "Entity.h"

namespace parus
{

    /** Owns every Entity in the current scene plus its optional components, keyed by EntityId. */
    class EntityManager final
    {
    public:
        /** Spawns a new entity; auto-suffixes requestedName if it's already taken. Returns the new entity's id. */
        EntityId spawn(const std::string& requestedName);

        /** Removes the entity with the given id. Returns false if no such entity exists. */
        bool destroy(EntityId id);

        const Entity* getEntity(EntityId id) const;
        const Entity* getEntityByName(const std::string& name) const;
        std::vector<const Entity*> getAllEntities() const;

        /** Removes every entity and component. Used when loading a new scene. */
        void clearSceneEntities();

        /** Silently no-ops if no such entity exists. */
        void setTransform(EntityId id, const math::Matrix4x4& transform);
        /** Silently no-ops if no such entity exists. */
        void setMobility(EntityId id, Mobility mobility);
        /** Renames an entity; fails (returns false) if newName is already taken by a different entity. */
        bool renameEntity(EntityId id, const std::string& newName);

        void addMeshComponent(EntityId id, MeshComponent component);
        /** Returns nullptr if the entity has no MeshComponent. */
        const MeshComponent* getMeshComponent(EntityId id) const;
        void removeMeshComponent(EntityId id);

        void addPointLightComponent(EntityId id, PointLightComponent component);
        /** Returns nullptr if the entity has no PointLightComponent. */
        const PointLightComponent* getPointLightComponent(EntityId id) const;
        void removePointLightComponent(EntityId id);

        /** Every entity that has a MeshComponent, paired with it. */
        std::vector<std::pair<const Entity*, const MeshComponent*>> getMeshEntities() const;
        /** Every entity that has a PointLightComponent, paired with it. */
        std::vector<std::pair<const Entity*, const PointLightComponent*>> getPointLightEntities() const;

        /** Attaches the sun to this entity. Only one entity is expected to hold this component at a time. */
        void addDirectionalLightComponent(EntityId id, DirectionalLightComponent component);
        /** Returns nullptr if no directional light is set. */
        const Entity* getDirectionalLightEntity() const;
        /** Returns nullptr if no directional light is set. */
        const DirectionalLightComponent* getDirectionalLightComponent() const;

        /** Attaches the sky to this entity. Only one entity is expected to hold this component at a time. */
        void addSkyboxComponent(EntityId id, SkyboxComponent component);
        /** Returns nullptr if no skybox is set. */
        const Entity* getSkyboxEntity() const;
        /** Returns nullptr if no skybox is set. */
        const SkyboxComponent* getSkyboxComponent() const;

    private:
        EntityId nextId = 1;
        std::unordered_map<EntityId, Entity> entities;
        std::unordered_map<std::string, EntityId> nameToId;
        std::unordered_map<EntityId, MeshComponent> meshComponents;
        std::unordered_map<EntityId, PointLightComponent> pointLightComponents;
        std::unordered_map<EntityId, DirectionalLightComponent> directionalLightComponents;
        std::unordered_map<EntityId, SkyboxComponent> skyboxComponents;

        /** Returns requestedName unchanged if free, otherwise appends the lowest free numeric suffix. */
        std::string makeUniqueName(const std::string& requestedName) const;
    };

}
