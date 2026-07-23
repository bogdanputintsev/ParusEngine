#include "EntityManager.h"

namespace parus
{

    EntityId EntityManager::spawn(const std::string& requestedName)
    {
        const EntityId id = nextId;
        ++nextId;

        Entity entity;
        entity.id = id;
        entity.name = makeUniqueName(requestedName);

        nameToId.insert_or_assign(entity.name, id);
        entities.insert_or_assign(id, entity);

        return id;
    }

    bool EntityManager::destroy(EntityId id)
    {
        const auto entityIterator = entities.find(id);
        if (entityIterator == entities.end())
        {
            return false;
        }

        nameToId.erase(entityIterator->second.name);
        entities.erase(entityIterator);
        meshComponents.erase(id);
        pointLightComponents.erase(id);
        directionalLightComponents.erase(id);
        skyboxComponents.erase(id);

        return true;
    }

    const Entity* EntityManager::getEntity(EntityId id) const
    {
        const auto entityIterator = entities.find(id);
        if (entityIterator == entities.end())
        {
            return nullptr;
        }

        return &entityIterator->second;
    }

    const Entity* EntityManager::getEntityByName(const std::string& name) const
    {
        const auto nameIterator = nameToId.find(name);
        if (nameIterator == nameToId.end())
        {
            return nullptr;
        }

        return getEntity(nameIterator->second);
    }

    std::vector<const Entity*> EntityManager::getAllEntities() const
    {
        std::vector<const Entity*> allEntities;
        allEntities.reserve(entities.size());

        for (const auto& [id, entity] : entities)
        {
            allEntities.push_back(&entity);
        }

        return allEntities;
    }

    void EntityManager::clearSceneEntities()
    {
        entities.clear();
        nameToId.clear();
        meshComponents.clear();
        pointLightComponents.clear();
        directionalLightComponents.clear();
        skyboxComponents.clear();
    }

    void EntityManager::setTransform(EntityId id, const math::Matrix4x4& transform)
    {
        const auto entityIterator = entities.find(id);
        if (entityIterator == entities.end())
        {
            return;
        }

        entityIterator->second.transform = transform;
    }

    void EntityManager::setMobility(EntityId id, Mobility mobility)
    {
        const auto entityIterator = entities.find(id);
        if (entityIterator == entities.end())
        {
            return;
        }

        entityIterator->second.mobility = mobility;
    }

    bool EntityManager::renameEntity(EntityId id, const std::string& newName)
    {
        const auto entityIterator = entities.find(id);
        if (entityIterator == entities.end())
        {
            return false;
        }

        if (nameToId.contains(newName) && nameToId.at(newName) != id)
        {
            return false;
        }

        nameToId.erase(entityIterator->second.name);
        entityIterator->second.name = newName;
        nameToId.insert_or_assign(newName, id);

        return true;
    }

    void EntityManager::addMeshComponent(EntityId id, MeshComponent component)
    {
        meshComponents.insert_or_assign(id, std::move(component));
    }

    const MeshComponent* EntityManager::getMeshComponent(EntityId id) const
    {
        const auto componentIterator = meshComponents.find(id);
        if (componentIterator == meshComponents.end())
        {
            return nullptr;
        }

        return &componentIterator->second;
    }

    void EntityManager::removeMeshComponent(EntityId id)
    {
        meshComponents.erase(id);
    }

    void EntityManager::addPointLightComponent(EntityId id, PointLightComponent component)
    {
        pointLightComponents.insert_or_assign(id, component);
    }

    const PointLightComponent* EntityManager::getPointLightComponent(EntityId id) const
    {
        const auto componentIterator = pointLightComponents.find(id);
        if (componentIterator == pointLightComponents.end())
        {
            return nullptr;
        }

        return &componentIterator->second;
    }

    void EntityManager::removePointLightComponent(EntityId id)
    {
        pointLightComponents.erase(id);
    }

    std::vector<std::pair<const Entity*, const MeshComponent*>> EntityManager::getMeshEntities() const
    {
        std::vector<std::pair<const Entity*, const MeshComponent*>> result;
        result.reserve(meshComponents.size());

        for (const auto& [id, component] : meshComponents)
        {
            if (const Entity* entity = getEntity(id))
            {
                result.emplace_back(entity, &component);
            }
        }

        return result;
    }

    std::vector<std::pair<const Entity*, const PointLightComponent*>> EntityManager::getPointLightEntities() const
    {
        std::vector<std::pair<const Entity*, const PointLightComponent*>> result;
        result.reserve(pointLightComponents.size());

        for (const auto& [id, component] : pointLightComponents)
        {
            if (const Entity* entity = getEntity(id))
            {
                result.emplace_back(entity, &component);
            }
        }

        return result;
    }

    void EntityManager::addDirectionalLightComponent(EntityId id, DirectionalLightComponent component)
    {
        directionalLightComponents.clear();
        directionalLightComponents.insert_or_assign(id, component);
    }

    const Entity* EntityManager::getDirectionalLightEntity() const
    {
        if (directionalLightComponents.empty())
        {
            return nullptr;
        }

        return getEntity(directionalLightComponents.begin()->first);
    }

    const DirectionalLightComponent* EntityManager::getDirectionalLightComponent() const
    {
        if (directionalLightComponents.empty())
        {
            return nullptr;
        }

        return &directionalLightComponents.begin()->second;
    }

    void EntityManager::addSkyboxComponent(EntityId id, SkyboxComponent component)
    {
        skyboxComponents.clear();
        skyboxComponents.insert_or_assign(id, std::move(component));
    }

    const Entity* EntityManager::getSkyboxEntity() const
    {
        if (skyboxComponents.empty())
        {
            return nullptr;
        }

        return getEntity(skyboxComponents.begin()->first);
    }

    const SkyboxComponent* EntityManager::getSkyboxComponent() const
    {
        if (skyboxComponents.empty())
        {
            return nullptr;
        }

        return &skyboxComponents.begin()->second;
    }

    std::string EntityManager::makeUniqueName(const std::string& requestedName) const
    {
        if (!nameToId.contains(requestedName))
        {
            return requestedName;
        }

        uint32_t suffix = 1;
        std::string candidateName = requestedName + std::to_string(suffix);

        while (nameToId.contains(candidateName))
        {
            ++suffix;
            candidateName = requestedName + std::to_string(suffix);
        }

        return candidateName;
    }

}
