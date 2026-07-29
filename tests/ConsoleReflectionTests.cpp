#include <gtest/gtest.h>

#include "engine/Event.h"
#include "services/Services.h"
#include "services/console/Console.h"
#include "services/console/reflection/ConsoleReflection.h"
#include "services/world/camera/SpectatorCamera.h"
#include "services/world/entity/EntityManager.h"

namespace parus
{
    namespace
    {
        std::shared_ptr<EntityManager> makeWorldWithConsole()
        {
            Services::registerService<EventSystem>(std::make_shared<EventSystem>());
            Services::registerService<Console>(std::make_shared<Console>());
            return std::make_shared<EntityManager>();
        }
    }

    TEST(ConsoleReflection, SetAndGetPositionRoundTrips)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);

        const EntityId id = entityManager->spawn("Door");
        console->submitCommand("set Door.position 1 2 3");

        const Entity* entity = entityManager->getEntity(id);
        ASSERT_NE(entity, nullptr);
        EXPECT_FLOAT_EQ(entity->transform.position.x, 1.0f);
        EXPECT_FLOAT_EQ(entity->transform.position.y, 2.0f);
        EXPECT_FLOAT_EQ(entity->transform.position.z, 3.0f);

        const std::string readBack = console->submitCommand("get Door.position");
        EXPECT_NE(readBack.find("1"), std::string::npos);
    }

    TEST(ConsoleReflection, AddressByIdWorks)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);

        const EntityId id = entityManager->spawn("Door");
        console->submitCommand("set #" + std::to_string(id) + ".mobility movable");

        EXPECT_EQ(entityManager->getEntity(id)->mobility, Mobility::Movable);
    }

    TEST(ConsoleReflection, SetPointLightFieldRoundTrips)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);

        const EntityId id = entityManager->spawn("Lamp");
        entityManager->addPointLightComponent(id, PointLightComponent{});
        console->submitCommand("set Lamp.PointLight.intensity 5");

        ASSERT_NE(entityManager->getPointLightComponent(id), nullptr);
        EXPECT_FLOAT_EQ(entityManager->getPointLightComponent(id)->intensity, 5.0f);
    }

    TEST(ConsoleReflection, UnknownTargetReportsError)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);

        const std::string result = console->submitCommand("get Ghost.position");
        EXPECT_NE(result.find("Ghost"), std::string::npos);
    }

    TEST(ConsoleReflection, CameraSpeedRoundTrips)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        SpectatorCamera camera;
        ConsoleReflection reflection(console, entityManager, &camera);

        console->submitCommand("set camera.speed 12");
        EXPECT_FLOAT_EQ(camera.getSpeed(), 12.0f);

        const std::string readBack = console->submitCommand("get camera.speed");
        EXPECT_NE(readBack.find("12"), std::string::npos);
    }

    TEST(ConsoleReflection, CompletesEntityNameAfterVerb)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);
        entityManager->spawn("Door");

        const std::string hint = console->hintNext("set Do");
        EXPECT_EQ(hint, "set Door");
    }

    TEST(ConsoleReflection, StaticTrieCompletionStillWorksAlongsideReflection)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        console->registerConsoleCommand("save", [](const std::vector<std::string>&, CommandContext&) {});
        ConsoleReflection reflection(console, entityManager);

        const std::string hint = console->hintNext("sav");
        EXPECT_EQ(hint, "save");
    }

    TEST(ConsoleReflection, CompletesPropertyAfterDot)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);
        entityManager->spawn("Door");

        const std::string hint = console->hintNext("set Door.pos");
        EXPECT_EQ(hint, "set Door.position");
    }

    TEST(ConsoleReflection, CompletesCameraPropertyAfterTrailingDot)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        SpectatorCamera camera;
        ConsoleReflection reflection(console, entityManager, &camera);

        const std::string hint = console->hintNext("get camera.");
        EXPECT_EQ(hint, "get camera.acceleration");
    }

    TEST(ConsoleReflection, CompletesEntityPropertyAfterTrailingDot)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);
        const EntityId id = entityManager->spawn("Lamp");
        entityManager->addPointLightComponent(id, PointLightComponent{});

        // Component names sort before lower-case property names, so PointLight comes first.
        const std::string hint = console->hintNext("set Lamp.");
        EXPECT_EQ(hint, "set Lamp.PointLight");
    }

    TEST(ConsoleReflection, CompletesComponentPropertyAfterTrailingDot)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);
        const EntityId id = entityManager->spawn("Lamp");
        entityManager->addPointLightComponent(id, PointLightComponent{});

        const std::string hint = console->hintNext("set Lamp.PointLight.");
        EXPECT_EQ(hint, "set Lamp.PointLight.color");
    }

    TEST(ConsoleReflection, TrailingDotIsRejectedByGet)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        SpectatorCamera camera;
        ConsoleReflection reflection(console, entityManager, &camera);

        EXPECT_EQ(console->submitCommand("get camera."), "Invalid address: camera.");
    }

    TEST(ConsoleReflection, CameraPositionRoundTrips)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        SpectatorCamera camera;
        ConsoleReflection reflection(console, entityManager, &camera);

        console->submitCommand("set camera.position 4 5 6");
        EXPECT_FLOAT_EQ(camera.getPosition().x, 4.0f);
        EXPECT_FLOAT_EQ(camera.getPosition().z, 6.0f);
    }

    TEST(ConsoleReflection, ListEntityPrintsFieldsAndComponents)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        ConsoleReflection reflection(console, entityManager);

        const EntityId id = entityManager->spawn("Lamp");
        entityManager->addPointLightComponent(id, PointLightComponent{});
        console->submitCommand("set Lamp.PointLight.intensity 5");

        const std::string result = console->submitCommand("list Lamp");
        EXPECT_NE(result.find("name = Lamp"), std::string::npos);
        EXPECT_NE(result.find("mobility ="), std::string::npos);
        EXPECT_NE(result.find("PointLight.intensity = 5"), std::string::npos);
    }

    TEST(ConsoleReflection, SettingCameraYawRecalculatesForwardVector)
    {
        auto entityManager = makeWorldWithConsole();
        auto console = Services::get<Console>();
        SpectatorCamera camera;
        ConsoleReflection reflection(console, entityManager, &camera);

        const math::Vector3 initialForward = camera.getForwardVector();

        console->submitCommand("set camera.yaw 90");
        const std::string forwardAfter = console->submitCommand("get camera.forward");

        const std::string initialForwardText = std::to_string(initialForward.x) + " "
            + std::to_string(initialForward.y) + " " + std::to_string(initialForward.z);
        EXPECT_NE(forwardAfter, initialForwardText);
    }
}
