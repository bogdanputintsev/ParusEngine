#include "World.h"

#include <stdexcept>

#include "services/Services.h"
#include "services/console/Console.h"

namespace parus
{
    void World::init()
    {
        registerConsoleCommands();
    }

    void World::tick(const float deltaTime)
    {
        mainCamera.updateTransform(deltaTime);
    }

    void World::registerConsoleCommands()
    {
        auto console = Services::get<Console>();

        console->registerConsoleCommand("print camera", [this](const auto&)
        {
            const auto position = mainCamera.getPosition();
            const auto forward = mainCamera.getForwardVector();
            return std::string("Camera:")
                + "\n\tPosition:     " + std::to_string(position.x) + ", " + std::to_string(position.y) + ", " + std::to_string(position.z)
                + "\n\tForward:      " + std::to_string(forward.x)  + ", " + std::to_string(forward.y)  + ", " + std::to_string(forward.z)
                + "\n\tYaw:          " + std::to_string(mainCamera.getYaw())
                + "\n\tPitch:        " + std::to_string(mainCamera.getPitch())
                + "\n\tSpeed:        " + std::to_string(mainCamera.getSpeed())
                + "\n\tAcceleration: " + std::to_string(mainCamera.getSpeedAccelerationMultiplier())
                + "\n\tSensitivity:  " + std::to_string(mainCamera.getSensitivity());
        });

        console->registerConsoleCommand("print camera position", [this](const auto&)
        {
            const auto position = mainCamera.getPosition();
            return std::string("Camera position:")
                + "\n\tX: " + std::to_string(position.x)
                + "\n\tY: " + std::to_string(position.y)
                + "\n\tZ: " + std::to_string(position.z);
        });

        console->registerConsoleCommand("print camera forward", [this](const auto&)
        {
            const auto forward = mainCamera.getForwardVector();
            return std::string("Camera forward:")
                + "\n\tX: " + std::to_string(forward.x)
                + "\n\tY: " + std::to_string(forward.y)
                + "\n\tZ: " + std::to_string(forward.z);
        });

        console->registerConsoleCommand("print camera rotation", [this](const auto&)
        {
            return std::string("Camera rotation:")
                + "\n\tYaw:   " + std::to_string(mainCamera.getYaw())
                + "\n\tPitch: " + std::to_string(mainCamera.getPitch());
        });

        console->registerConsoleCommand("print camera rotation yaw", [this](const auto&)
        {
            return std::string("Camera yaw:\n\t") + std::to_string(mainCamera.getYaw());
        });

        console->registerConsoleCommand("print camera rotation pitch", [this](const auto&)
        {
            return std::string("Camera pitch:\n\t") + std::to_string(mainCamera.getPitch());
        });

        console->registerConsoleCommand("print camera sensitivity", [this](const auto&)
        {
            return std::string("Camera sensitivity:\n\t") + std::to_string(mainCamera.getSensitivity());
        });

        console->registerConsoleCommand("print camera speed", [this](const auto&)
        {
            return std::string("Camera speed:\n\t") + std::to_string(mainCamera.getSpeed());
        });

        console->registerConsoleCommand("print camera acceleration", [this](const auto&)
        {
            return std::string("Camera speed acceleration:\n\t") + std::to_string(mainCamera.getSpeedAccelerationMultiplier());
        });

        console->registerConsoleCommand("set camera position", [this](const std::vector<std::string>& args)
        {
            if (args.size() < 3)
            {
                return std::string("Usage: set camera position <x> <y> <z>");
            }
            try
            {
                mainCamera.setPosition({ std::stof(args[0]), std::stof(args[1]), std::stof(args[2]) });
                return std::string("Camera position set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid arguments. Expected three numbers.");
            }
        });

        console->registerConsoleCommand("set camera rotation", [this](const std::vector<std::string>& args)
        {
            if (args.size() < 2)
            {
                return std::string("Usage: set camera rotation <yaw> <pitch>");
            }
            try
            {
                mainCamera.setYaw(std::stof(args[0]));
                mainCamera.setPitch(std::stof(args[1]));
                return std::string("Camera rotation set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid arguments. Expected two numbers.");
            }
        });

        console->registerConsoleCommand("set camera rotation yaw", [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                return std::string("Usage: set camera rotation yaw <value>");
            }
            try
            {
                mainCamera.setYaw(std::stof(args[0]));
                return std::string("Camera yaw set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid argument. Expected a number.");
            }
        });

        console->registerConsoleCommand("set camera rotation pitch", [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                return std::string("Usage: set camera rotation pitch <value>");
            }
            try
            {
                mainCamera.setPitch(std::stof(args[0]));
                return std::string("Camera pitch set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid argument. Expected a number.");
            }
        });

        console->registerConsoleCommand("set camera sensitivity", [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                return std::string("Usage: set camera sensitivity <value>");
            }
            try
            {
                mainCamera.setSensitivity(std::stof(args[0]));
                return std::string("Camera sensitivity set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid argument. Expected a number.");
            }
        });

        console->registerConsoleCommand("set camera speed", [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                return std::string("Usage: set camera speed <value>");
            }
            try
            {
                mainCamera.setSpeed(std::stof(args[0]));
                return std::string("Camera speed set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid argument. Expected a number.");
            }
        });

        console->registerConsoleCommand("set camera acceleration", [this](const std::vector<std::string>& args)
        {
            if (args.empty())
            {
                return std::string("Usage: set camera speed acceleration <value>");
            }
            try
            {
                mainCamera.setSpeedAccelerationMultiplier(std::stof(args[0]));
                return std::string("Camera speed acceleration set.");
            }
            catch (const std::invalid_argument&)
            {
                return std::string("Invalid argument. Expected a number.");
            }
        });
    }

}
