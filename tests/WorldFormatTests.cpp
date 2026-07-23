#include <gtest/gtest.h>

#include <filesystem>

#include "services/renderer/vulkan/mesh/Mesh.h"
#include "services/serialization/WorldFormat.h"
#include "services/world/World.h"
#include "services/world/entity/Components.h"

namespace parus::serialization
{
    namespace
    {
        std::shared_ptr<Mesh> makeGeometryMesh(const std::string& stem)
        {
            auto mesh = std::make_shared<Mesh>();
            mesh->meshType = MeshType::GEOMETRY;
            mesh->sourcePath = stem;

            return mesh;
        }
    }

    TEST(WorldFormatRoundTrip, WriteThenReadPreservesEntitiesAndComponents)
    {
        World world;
        const auto entityManager = world.getEntityManager();
        const auto storage = world.getStorage();

        const auto cubeMesh = makeGeometryMesh("cube");
        storage->addNewMesh("cube", cubeMesh);

        const EntityId cubeId = entityManager->spawn("Cube");
        entityManager->setMobility(cubeId, Mobility::Movable);
        entityManager->setTransform(cubeId, math::Matrix4x4::translation(1.0f, 2.0f, 3.0f));
        entityManager->addMeshComponent(cubeId, MeshComponent{ cubeMesh });

        const EntityId lampId = entityManager->spawn("Lamp");
        entityManager->addPointLightComponent(lampId, PointLightComponent{
            math::Vector3(1.0f, 0.9f, 0.8f), 25.0f, 3.0f
        });

        const EntityId sunId = entityManager->spawn("Sun");
        entityManager->addDirectionalLightComponent(sunId, DirectionalLightComponent{
            math::Vector3(1.0f, 1.0f, 0.95f), math::Vector3(0.0f, -1.0f, 0.2f)
        });

        const auto skyMesh = makeGeometryMesh("sky");
        skyMesh->meshType = MeshType::SKY;
        storage->addNewMesh("sky", skyMesh);
        const EntityId skyId = entityManager->spawn("Skybox");
        entityManager->addSkyboxComponent(skyId, SkyboxComponent{
            skyMesh, math::Vector3(0.7f, 0.8f, 1.0f), math::Vector3(0.05f, 0.1f, 0.4f)
        });

        world.setCameraTransform(math::Vector3(10.0f, 20.0f, 30.0f), 0.5f, -0.2f);

        const std::filesystem::path scenesDir = std::filesystem::temp_directory_path() / "parus_world_format_test";
        std::filesystem::create_directories(scenesDir);
        const std::string sceneName = "roundtrip_test_scene";

        writeWorld(world, sceneName, scenesDir / (sceneName + ".pworld"));

        const std::optional<SceneData> loaded = readWorld(sceneName, scenesDir);
        ASSERT_TRUE(loaded.has_value());

        EXPECT_EQ(loaded->cameraPosition, math::Vector3(10.0f, 20.0f, 30.0f));
        EXPECT_FLOAT_EQ(loaded->cameraYaw, 0.5f);
        EXPECT_FLOAT_EQ(loaded->cameraPitch, -0.2f);

        EXPECT_EQ(loaded->directionalLight.color, math::Vector3(1.0f, 1.0f, 0.95f));
        EXPECT_EQ(loaded->directionalLight.direction, math::Vector3(0.0f, -1.0f, 0.2f));

        EXPECT_EQ(loaded->skybox.meshStem, "sky");
        EXPECT_EQ(loaded->skybox.horizonColor, math::Vector3(0.7f, 0.8f, 1.0f));
        EXPECT_EQ(loaded->skybox.zenithColor, math::Vector3(0.05f, 0.1f, 0.4f));

        ASSERT_EQ(loaded->meshStems.size(), 1u);
        EXPECT_EQ(loaded->meshStems[0], "cube");

        ASSERT_EQ(loaded->entities.size(), 2u);

        const auto findEntry = [&](const std::string& name) -> const EntityEntry*
        {
            for (const auto& entry : loaded->entities)
            {
                if (entry.name == name)
                {
                    return &entry;
                }
            }

            return nullptr;
        };

        const EntityEntry* cubeEntry = findEntry("Cube");
        ASSERT_NE(cubeEntry, nullptr);
        EXPECT_EQ(cubeEntry->mobility, Mobility::Movable);
        EXPECT_EQ(cubeEntry->transform, math::Matrix4x4::translation(1.0f, 2.0f, 3.0f));
        ASSERT_TRUE(cubeEntry->meshComponent.has_value());
        EXPECT_EQ(cubeEntry->meshComponent->meshIndex, 0u);
        EXPECT_FALSE(cubeEntry->pointLightComponent.has_value());

        const EntityEntry* lampEntry = findEntry("Lamp");
        ASSERT_NE(lampEntry, nullptr);
        EXPECT_EQ(lampEntry->mobility, Mobility::Static);
        ASSERT_TRUE(lampEntry->pointLightComponent.has_value());
        EXPECT_EQ(lampEntry->pointLightComponent->color, math::Vector3(1.0f, 0.9f, 0.8f));
        EXPECT_FLOAT_EQ(lampEntry->pointLightComponent->radius, 25.0f);
        EXPECT_FLOAT_EQ(lampEntry->pointLightComponent->intensity, 3.0f);
        EXPECT_FALSE(lampEntry->meshComponent.has_value());

        std::filesystem::remove_all(scenesDir);
    }

    // Regression test: a scene whose skybox has no sourcePath (the built-in sky mesh never sets
    // one) writes an empty mesh stem. Storage::getMeshByPath must not insert a null entry for that
    // (or any other) missing key - doing so previously corrupted Storage::meshes with a null
    // shared_ptr that later crashed Storage::getAllMeshes/getAllMeshesByType on dereference.
    TEST(StorageGetMeshByPath, MissingKeyReturnsNullWithoutInsertingEntry)
    {
        Storage storage;

        const std::shared_ptr<Mesh> result = storage.getMeshByPath("");

        EXPECT_EQ(result, nullptr);
        EXPECT_TRUE(storage.getAllMeshes().empty());
    }
}
