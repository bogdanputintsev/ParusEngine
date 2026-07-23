#include <gtest/gtest.h>

#include "services/world/entity/EntityManager.h"
#include "services/world/entity/Components.h"

namespace parus
{
    TEST(EntityManager, SpawnAssignsIncrementingIds)
    {
        EntityManager entityManager;

        const EntityId first  = entityManager.spawn("Cube");
        const EntityId second = entityManager.spawn("Sphere");

        EXPECT_NE(first, second);
    }

    TEST(EntityManager, SpawnedEntityIsRetrievableById)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const Entity* entity = entityManager.getEntity(id);

        ASSERT_NE(entity, nullptr);
        EXPECT_EQ(entity->id, id);
        EXPECT_EQ(entity->name, "Cube");
        EXPECT_EQ(entity->mobility, Mobility::Static);
    }

    TEST(EntityManager, SpawnedEntityIsRetrievableByName)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const Entity* entity = entityManager.getEntityByName("Cube");

        ASSERT_NE(entity, nullptr);
        EXPECT_EQ(entity->id, id);
    }

    TEST(EntityManager, GetEntityReturnsNullForUnknownId)
    {
        EntityManager entityManager;

        EXPECT_EQ(entityManager.getEntity(999), nullptr);
    }

    TEST(EntityManager, GetEntityByNameReturnsNullForUnknownName)
    {
        EntityManager entityManager;

        EXPECT_EQ(entityManager.getEntityByName("Missing"), nullptr);
    }

    TEST(EntityManager, DuplicateNameGetsAutoSuffixed)
    {
        EntityManager entityManager;

        entityManager.spawn("Cube");
        const EntityId secondId = entityManager.spawn("Cube");
        const EntityId thirdId  = entityManager.spawn("Cube");

        EXPECT_EQ(entityManager.getEntity(secondId)->name, "Cube1");
        EXPECT_EQ(entityManager.getEntity(thirdId)->name, "Cube2");
    }

    TEST(EntityManager, DestroyRemovesEntity)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const bool destroyed = entityManager.destroy(id);

        EXPECT_TRUE(destroyed);
        EXPECT_EQ(entityManager.getEntity(id), nullptr);
        EXPECT_EQ(entityManager.getEntityByName("Cube"), nullptr);
    }

    TEST(EntityManager, DestroyReturnsFalseForUnknownId)
    {
        EntityManager entityManager;

        EXPECT_FALSE(entityManager.destroy(999));
    }

    TEST(EntityManager, GetAllEntitiesReturnsEverySpawnedEntity)
    {
        EntityManager entityManager;

        entityManager.spawn("Cube");
        entityManager.spawn("Sphere");

        const std::vector<const Entity*> all = entityManager.getAllEntities();

        EXPECT_EQ(all.size(), 2u);
    }

    TEST(EntityManager, ClearSceneEntitiesRemovesEverything)
    {
        EntityManager entityManager;

        entityManager.spawn("Cube");
        entityManager.spawn("Sphere");
        entityManager.clearSceneEntities();

        EXPECT_TRUE(entityManager.getAllEntities().empty());
    }

    TEST(EntityManager, IdsAreNeverReusedAfterDestroy)
    {
        EntityManager entityManager;

        const EntityId first = entityManager.spawn("Cube");
        entityManager.destroy(first);
        const EntityId second = entityManager.spawn("Cube");

        EXPECT_NE(first, second);
    }

    TEST(EntityManager, SetTransformUpdatesEntity)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const math::Matrix4x4 newTransform = math::Matrix4x4::translation(1.0f, 2.0f, 3.0f);
        entityManager.setTransform(id, newTransform);

        EXPECT_EQ(entityManager.getEntity(id)->transform, newTransform);
    }

    TEST(EntityManager, SetMobilityUpdatesEntity)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        entityManager.setMobility(id, Mobility::Movable);

        EXPECT_EQ(entityManager.getEntity(id)->mobility, Mobility::Movable);
    }

    TEST(EntityManager, RenameEntitySucceedsWithUniqueName)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const bool renamed = entityManager.renameEntity(id, "Box");

        EXPECT_TRUE(renamed);
        EXPECT_EQ(entityManager.getEntity(id)->name, "Box");
        EXPECT_EQ(entityManager.getEntityByName("Cube"), nullptr);
        EXPECT_NE(entityManager.getEntityByName("Box"), nullptr);
    }

    TEST(EntityManager, RenameEntityFailsForUnknownId)
    {
        EntityManager entityManager;

        EXPECT_FALSE(entityManager.renameEntity(999, "Box"));
    }

    TEST(EntityManager, AddAndGetMeshComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        const auto mesh = std::make_shared<Mesh>();
        entityManager.addMeshComponent(id, MeshComponent{ mesh });

        const MeshComponent* component = entityManager.getMeshComponent(id);
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->mesh, mesh);
    }

    TEST(EntityManager, GetMeshComponentReturnsNullWhenAbsent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");

        EXPECT_EQ(entityManager.getMeshComponent(id), nullptr);
    }

    TEST(EntityManager, RemoveMeshComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Cube");
        entityManager.addMeshComponent(id, MeshComponent{ std::make_shared<Mesh>() });
        entityManager.removeMeshComponent(id);

        EXPECT_EQ(entityManager.getMeshComponent(id), nullptr);
    }

    TEST(EntityManager, GetMeshEntitiesReturnsOnlyEntitiesWithMesh)
    {
        EntityManager entityManager;

        const EntityId meshEntityId = entityManager.spawn("Cube");
        entityManager.spawn("Empty");
        entityManager.addMeshComponent(meshEntityId, MeshComponent{ std::make_shared<Mesh>() });

        const auto meshEntities = entityManager.getMeshEntities();

        ASSERT_EQ(meshEntities.size(), 1u);
        EXPECT_EQ(meshEntities[0].first->id, meshEntityId);
    }

    TEST(EntityManager, AddAndGetPointLightComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Lamp");
        const PointLightComponent light{ math::Vector3(1.0f, 0.5f, 0.25f), 10.0f, 2.0f };
        entityManager.addPointLightComponent(id, light);

        const PointLightComponent* component = entityManager.getPointLightComponent(id);
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->color, light.color);
        EXPECT_FLOAT_EQ(component->radius, light.radius);
        EXPECT_FLOAT_EQ(component->intensity, light.intensity);
    }

    TEST(EntityManager, RemovePointLightComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Lamp");
        entityManager.addPointLightComponent(id, PointLightComponent{});
        entityManager.removePointLightComponent(id);

        EXPECT_EQ(entityManager.getPointLightComponent(id), nullptr);
    }

    TEST(EntityManager, GetPointLightEntitiesReturnsOnlyEntitiesWithPointLight)
    {
        EntityManager entityManager;

        const EntityId lampId = entityManager.spawn("Lamp");
        entityManager.spawn("Empty");
        entityManager.addPointLightComponent(lampId, PointLightComponent{});

        const auto pointLightEntities = entityManager.getPointLightEntities();

        ASSERT_EQ(pointLightEntities.size(), 1u);
        EXPECT_EQ(pointLightEntities[0].first->id, lampId);
    }

    TEST(EntityManager, AddAndGetDirectionalLightComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Sun");
        const DirectionalLightComponent light{ math::Vector3(1.0f, 1.0f, 1.0f), math::Vector3(0.0f, -1.0f, 0.0f) };
        entityManager.addDirectionalLightComponent(id, light);

        ASSERT_NE(entityManager.getDirectionalLightEntity(), nullptr);
        EXPECT_EQ(entityManager.getDirectionalLightEntity()->id, id);

        const DirectionalLightComponent* component = entityManager.getDirectionalLightComponent();
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->color, light.color);
        EXPECT_EQ(component->direction, light.direction);
    }

    TEST(EntityManager, GetDirectionalLightEntityReturnsNullWhenNoneSet)
    {
        EntityManager entityManager;

        EXPECT_EQ(entityManager.getDirectionalLightEntity(), nullptr);
        EXPECT_EQ(entityManager.getDirectionalLightComponent(), nullptr);
    }

    TEST(EntityManager, AddAndGetSkyboxComponent)
    {
        EntityManager entityManager;

        const EntityId id = entityManager.spawn("Sky");
        const auto mesh = std::make_shared<Mesh>();
        const SkyboxComponent skybox{ mesh, math::Vector3(0.8f, 0.8f, 1.0f), math::Vector3(0.1f, 0.2f, 0.6f) };
        entityManager.addSkyboxComponent(id, skybox);

        ASSERT_NE(entityManager.getSkyboxEntity(), nullptr);
        EXPECT_EQ(entityManager.getSkyboxEntity()->id, id);

        const SkyboxComponent* component = entityManager.getSkyboxComponent();
        ASSERT_NE(component, nullptr);
        EXPECT_EQ(component->mesh, mesh);
        EXPECT_EQ(component->horizonColor, skybox.horizonColor);
        EXPECT_EQ(component->zenithColor, skybox.zenithColor);
    }

    TEST(EntityManager, GetSkyboxEntityReturnsNullWhenNoneSet)
    {
        EntityManager entityManager;

        EXPECT_EQ(entityManager.getSkyboxEntity(), nullptr);
        EXPECT_EQ(entityManager.getSkyboxComponent(), nullptr);
    }

    TEST(EntityManager, ClearSceneEntitiesRemovesDirectionalLightAndSkybox)
    {
        EntityManager entityManager;

        const EntityId sunId = entityManager.spawn("Sun");
        entityManager.addDirectionalLightComponent(sunId, DirectionalLightComponent{});
        const EntityId skyId = entityManager.spawn("Sky");
        entityManager.addSkyboxComponent(skyId, SkyboxComponent{});

        entityManager.clearSceneEntities();

        EXPECT_EQ(entityManager.getDirectionalLightEntity(), nullptr);
        EXPECT_EQ(entityManager.getSkyboxEntity(), nullptr);
    }
}
