#include "ConsoleReflection.h"

#include <algorithm>
#include <stdexcept>

#include "services/console/CommandContext.h"
#include "services/console/Console.h"
#include "services/world/camera/SpectatorCamera.h"
#include "services/world/entity/Components.h"
#include "services/world/entity/EntityManager.h"

namespace parus
{
    namespace
    {
        constexpr char ID_PREFIX = '#';

        /** Separates the segments of an address: "Lamp.PointLight.intensity". */
        constexpr char SEGMENT_SEPARATOR = '.';

        /** Special target name that routes to the world's camera instead of an entity. */
        constexpr const char* CAMERA_TARGET_NAME = "camera";

        /** Component schema names, in the order handleList should print them. */
        const std::vector<std::string> COMPONENT_SCHEMA_NAMES = { "PointLight", "DirectionalLight", "Skybox", "Mesh" };

        bool parseVector3(const std::vector<std::string>& args, math::Vector3& outVector, std::string& error)
        {
            if (args.size() < 3)
            {
                error = "expected three numbers";
                return false;
            }
            try
            {
                outVector = { std::stof(args[0]), std::stof(args[1]), std::stof(args[2]) };
                return true;
            }
            catch (const std::exception&)
            {
                error = "expected three numbers";
                return false;
            }
        }

        std::string formatVector3(const math::Vector3& vector)
        {
            return std::to_string(vector.x) + " " + std::to_string(vector.y) + " " + std::to_string(vector.z);
        }

        bool parseFloat(const std::vector<std::string>& args, float& outValue, std::string& error)
        {
            if (args.empty())
            {
                error = "expected a number";
                return false;
            }
            try
            {
                outValue = std::stof(args[0]);
                return true;
            }
            catch (const std::exception&)
            {
                error = "expected a number";
                return false;
            }
        }

        /**
         * Returns a read-only pointer to the entity's live component storage for the given
         * component schema name, or nullptr if the entity does not hold that component.
         * DirectionalLight and Skybox are scene-wide singletons, so ownership is confirmed
         * by comparing the resolved id against the entity that currently holds them.
         */
        const void* resolveComponentPointer(const EntityManager& entityManager, EntityId id, const std::string& componentName)
        {
            if (componentName == "PointLight")
            {
                return entityManager.getPointLightComponent(id);
            }
            if (componentName == "DirectionalLight")
            {
                const Entity* directionalLightEntity = entityManager.getDirectionalLightEntity();
                if (!directionalLightEntity || directionalLightEntity->id != id)
                {
                    return nullptr;
                }
                return entityManager.getDirectionalLightComponent();
            }
            if (componentName == "Skybox")
            {
                const Entity* skyboxEntity = entityManager.getSkyboxEntity();
                if (!skyboxEntity || skyboxEntity->id != id)
                {
                    return nullptr;
                }
                return entityManager.getSkyboxComponent();
            }
            if (componentName == "Mesh")
            {
                return entityManager.getMeshComponent(id);
            }

            return nullptr;
        }

        /** Reads a "camera.<property>" value. Returns nullopt for an unknown property name. */
        std::optional<std::string> readCameraProperty(const SpectatorCamera& camera, const std::string& property)
        {
            if (property == "position")
            {
                return formatVector3(camera.getPosition());
            }
            if (property == "forward")
            {
                return formatVector3(camera.getForwardVector());
            }
            if (property == "yaw")
            {
                return std::to_string(camera.getYaw());
            }
            if (property == "pitch")
            {
                return std::to_string(camera.getPitch());
            }
            if (property == "speed")
            {
                return std::to_string(camera.getSpeed());
            }
            if (property == "sensitivity")
            {
                return std::to_string(camera.getSensitivity());
            }
            if (property == "acceleration")
            {
                return std::to_string(camera.getSpeedAccelerationMultiplier());
            }

            return std::nullopt;
        }

        /**
         * Writes a "camera.<property>" value. Yaw and pitch recalculate the camera's
         * direction vectors immediately so a subsequent "get camera.forward" reflects them.
         */
        bool writeCameraProperty(SpectatorCamera& camera, const std::string& property, const std::vector<std::string>& values, std::string& error)
        {
            if (property == "position")
            {
                math::Vector3 position;
                if (!parseVector3(values, position, error))
                {
                    return false;
                }
                camera.setPosition(position);
                return true;
            }
            if (property == "yaw")
            {
                float yaw = 0.0f;
                if (!parseFloat(values, yaw, error))
                {
                    return false;
                }
                camera.setYaw(yaw);
                camera.recalculateDirections();
                return true;
            }
            if (property == "pitch")
            {
                float pitch = 0.0f;
                if (!parseFloat(values, pitch, error))
                {
                    return false;
                }
                camera.setPitch(pitch);
                camera.recalculateDirections();
                return true;
            }
            if (property == "speed")
            {
                float speed = 0.0f;
                if (!parseFloat(values, speed, error))
                {
                    return false;
                }
                camera.setSpeed(speed);
                return true;
            }
            if (property == "sensitivity")
            {
                float sensitivity = 0.0f;
                if (!parseFloat(values, sensitivity, error))
                {
                    return false;
                }
                camera.setSensitivity(sensitivity);
                return true;
            }
            if (property == "acceleration")
            {
                float acceleration = 0.0f;
                if (!parseFloat(values, acceleration, error))
                {
                    return false;
                }
                camera.setSpeedAccelerationMultiplier(acceleration);
                return true;
            }
            if (property == "forward")
            {
                error = "camera.forward is read-only";
                return false;
            }

            error = "Unknown property: " + property;
            return false;
        }

        /** Property names listed by "list camera", in display order. */
        const std::vector<std::string> CAMERA_PROPERTY_NAMES = { "position", "yaw", "pitch", "speed", "sensitivity", "acceleration", "forward" };

        /**
         * Completes currentWord against sortedCandidates, mirroring Trie::cycleCurrentWord:
         * an exact match advances to the next candidate (cycling back to the first), otherwise
         * the first candidate alphabetically at or after currentWord is used if it is a prefix match.
         */
        std::optional<std::string> cycleWord(const std::vector<std::string>& sortedCandidates, const std::string& currentWord)
        {
            const auto exactMatch = std::find(sortedCandidates.begin(), sortedCandidates.end(), currentWord);
            if (exactMatch != sortedCandidates.end())
            {
                auto next = std::next(exactMatch);
                if (next == sortedCandidates.end())
                {
                    next = sortedCandidates.begin();
                }
                return *next;
            }

            const auto prefixMatch = std::lower_bound(sortedCandidates.begin(), sortedCandidates.end(), currentWord);
            if (prefixMatch != sortedCandidates.end() && prefixMatch->starts_with(currentWord))
            {
                return *prefixMatch;
            }

            return std::nullopt;
        }
    }

    ConsoleReflection::ConsoleReflection(std::shared_ptr<Console> console, std::shared_ptr<EntityManager> entityManager, SpectatorCamera* camera)
        : console(std::move(console))
        , entityManager(std::move(entityManager))
        , camera(camera)
    {
        buildEntitySchemas();
        registerCommands();

        this->console->registerCompletionProvider([this](const std::string& input)
        {
            return completeAddress(input);
        });
    }

    std::optional<EntityId> ConsoleReflection::resolveEntityId(const std::string& targetToken) const
    {
        if (!targetToken.empty() && targetToken.front() == ID_PREFIX)
        {
            try
            {
                const EntityId id = static_cast<EntityId>(std::stoul(targetToken.substr(1)));
                if (entityManager->getEntity(id))
                {
                    return id;
                }
            }
            catch (const std::exception&)
            {
                return std::nullopt;
            }
            return std::nullopt;
        }

        if (const Entity* entity = entityManager->getEntityByName(targetToken))
        {
            return entity->id;
        }

        return std::nullopt;
    }

    std::vector<std::string> ConsoleReflection::splitAddress(const std::string& address)
    {
        // Empty segments are preserved: "camera." yields { "camera", "" }, which is what
        // tab-completion needs to offer every property of a target the user just dotted into.
        std::vector<std::string> segments;
        std::string segment;
        for (const char character : address)
        {
            if (character == SEGMENT_SEPARATOR)
            {
                segments.push_back(segment);
                segment.clear();
                continue;
            }

            segment += character;
        }
        segments.push_back(segment);

        return segments;
    }

    bool ConsoleReflection::hasEmptySegment(const std::vector<std::string>& address)
    {
        return std::any_of(address.begin(), address.end(), [](const std::string& segment) { return segment.empty(); });
    }

    std::optional<std::string> ConsoleReflection::completeAddress(const std::string& input) const
    {
        constexpr const char* VERB_PREFIXES[] = { "get ", "set ", "list " };
        const bool startsWithVerb = std::any_of(std::begin(VERB_PREFIXES), std::end(VERB_PREFIXES),
            [&input](const char* verb) { return input.starts_with(verb); });
        if (!startsWithVerb)
        {
            return std::nullopt;
        }

        const size_t lastSpace = input.find_last_of(' ');
        const std::string linePrefix = input.substr(0, lastSpace + 1);
        const std::string lastToken = input.substr(lastSpace + 1);

        if (lastToken.find(SEGMENT_SEPARATOR) == std::string::npos)
        {
            std::vector<std::string> candidates;
            for (const Entity* entity : entityManager->getAllEntities())
            {
                candidates.push_back(entity->name);
            }
            if (camera)
            {
                candidates.push_back(CAMERA_TARGET_NAME);
            }
            std::sort(candidates.begin(), candidates.end());

            const std::optional<std::string> completed = cycleWord(candidates, lastToken);
            if (!completed)
            {
                return std::nullopt;
            }

            return linePrefix + *completed;
        }

        // splitAddress always yields at least one segment, so indexing here is safe.
        const std::vector<std::string> segments = splitAddress(lastToken);
        const std::string& targetName = segments[0];
        const std::string& lastSegment = segments.back();
        const std::string fixedPart = lastToken.substr(0, lastToken.size() - lastSegment.size());

        std::vector<std::string> candidates;

        if (targetName == CAMERA_TARGET_NAME && camera)
        {
            if (segments.size() != 2)
            {
                return std::nullopt;
            }
            candidates = CAMERA_PROPERTY_NAMES;
        }
        else
        {
            const std::optional<EntityId> id = resolveEntityId(targetName);
            if (!id)
            {
                return std::nullopt;
            }

            if (segments.size() == 2)
            {
                const TargetSchema* entitySchema = registry.findSchema("Entity");
                for (const PropertyAccessor& property : entitySchema->properties)
                {
                    candidates.push_back(property.name);
                }
                for (const std::string& componentName : COMPONENT_SCHEMA_NAMES)
                {
                    if (resolveComponentPointer(*entityManager, *id, componentName))
                    {
                        candidates.push_back(componentName);
                    }
                }
            }
            else if (segments.size() == 3)
            {
                const std::string& componentName = segments[1];
                const TargetSchema* componentSchema = registry.findSchema(componentName);
                if (!componentSchema || !resolveComponentPointer(*entityManager, *id, componentName))
                {
                    return std::nullopt;
                }
                for (const PropertyAccessor& property : componentSchema->properties)
                {
                    candidates.push_back(property.name);
                }
            }
            else
            {
                return std::nullopt;
            }
        }

        std::sort(candidates.begin(), candidates.end());
        const std::optional<std::string> completed = cycleWord(candidates, lastSegment);
        if (!completed)
        {
            return std::nullopt;
        }

        return linePrefix + fixedPart + *completed;
    }

    void ConsoleReflection::buildEntitySchemas()
    {
        buildEntityIntrinsicsSchema();
        buildPointLightSchema();
        buildDirectionalLightSchema();
        buildSkyboxSchema();
        buildMeshSchema();
    }

    void ConsoleReflection::buildEntityIntrinsicsSchema()
    {
        TargetSchema entitySchema;
        entitySchema.name = "Entity";
        entitySchema.properties.push_back(PropertyAccessor{
            "name", "string",
            [](const void* object) { return static_cast<const Entity*>(object)->name; },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                if (values.empty())
                {
                    error = "expected a name";
                    return false;
                }
                static_cast<Entity*>(object)->name = values[0];
                return true;
            }
        });
        entitySchema.properties.push_back(PropertyAccessor{
            "mobility", "string",
            [](const void* object)
            {
                return static_cast<const Entity*>(object)->mobility == Mobility::Movable ? "movable" : "static";
            },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                if (values.empty())
                {
                    error = "expected static or movable";
                    return false;
                }
                if (values[0] == "movable")
                {
                    static_cast<Entity*>(object)->mobility = Mobility::Movable;
                    return true;
                }
                if (values[0] == "static")
                {
                    static_cast<Entity*>(object)->mobility = Mobility::Static;
                    return true;
                }
                error = "expected static or movable";
                return false;
            }
        });
        entitySchema.properties.push_back(PropertyAccessor{
            "position", "vec3",
            [](const void* object) { return formatVector3(static_cast<const Entity*>(object)->transform.position); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 position;
                if (!parseVector3(values, position, error))
                {
                    return false;
                }
                static_cast<Entity*>(object)->transform.position = position;
                return true;
            }
        });
        entitySchema.properties.push_back(PropertyAccessor{
            "rotation", "vec3",
            [](const void* object) { return formatVector3(static_cast<const Entity*>(object)->transform.rotationEuler); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 rotation;
                if (!parseVector3(values, rotation, error))
                {
                    return false;
                }
                static_cast<Entity*>(object)->transform.rotationEuler = rotation;
                return true;
            }
        });
        entitySchema.properties.push_back(PropertyAccessor{
            "scale", "vec3",
            [](const void* object) { return formatVector3(static_cast<const Entity*>(object)->transform.scale); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                // A single number applies uniformly to all three axes.
                if (values.size() == 1)
                {
                    float uniformScale = 0.0f;
                    if (!parseFloat(values, uniformScale, error))
                    {
                        return false;
                    }
                    static_cast<Entity*>(object)->transform.scale = { uniformScale, uniformScale, uniformScale };
                    return true;
                }

                math::Vector3 scale;
                if (!parseVector3(values, scale, error))
                {
                    return false;
                }
                static_cast<Entity*>(object)->transform.scale = scale;
                return true;
            }
        });
        registry.registerSchema(std::move(entitySchema));
    }

    void ConsoleReflection::buildPointLightSchema()
    {
        TargetSchema pointLightSchema;
        pointLightSchema.name = "PointLight";
        pointLightSchema.properties.push_back(PropertyAccessor{
            "color", "vec3",
            [](const void* object) { return formatVector3(static_cast<const PointLightComponent*>(object)->color); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 color;
                if (!parseVector3(values, color, error))
                {
                    return false;
                }
                static_cast<PointLightComponent*>(object)->color = color;
                return true;
            }
        });
        pointLightSchema.properties.push_back(PropertyAccessor{
            "radius", "float",
            [](const void* object) { return std::to_string(static_cast<const PointLightComponent*>(object)->radius); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                float radius = 0.0f;
                if (!parseFloat(values, radius, error))
                {
                    return false;
                }
                static_cast<PointLightComponent*>(object)->radius = radius;
                return true;
            }
        });
        pointLightSchema.properties.push_back(PropertyAccessor{
            "intensity", "float",
            [](const void* object) { return std::to_string(static_cast<const PointLightComponent*>(object)->intensity); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                float intensity = 0.0f;
                if (!parseFloat(values, intensity, error))
                {
                    return false;
                }
                static_cast<PointLightComponent*>(object)->intensity = intensity;
                return true;
            }
        });
        registry.registerSchema(std::move(pointLightSchema));
    }

    void ConsoleReflection::buildDirectionalLightSchema()
    {
        TargetSchema directionalLightSchema;
        directionalLightSchema.name = "DirectionalLight";
        directionalLightSchema.properties.push_back(PropertyAccessor{
            "color", "vec3",
            [](const void* object) { return formatVector3(static_cast<const DirectionalLightComponent*>(object)->color); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 color;
                if (!parseVector3(values, color, error))
                {
                    return false;
                }
                static_cast<DirectionalLightComponent*>(object)->color = color;
                return true;
            }
        });
        directionalLightSchema.properties.push_back(PropertyAccessor{
            "direction", "vec3",
            [](const void* object) { return formatVector3(static_cast<const DirectionalLightComponent*>(object)->direction); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 direction;
                if (!parseVector3(values, direction, error))
                {
                    return false;
                }
                static_cast<DirectionalLightComponent*>(object)->direction = direction;
                return true;
            }
        });
        registry.registerSchema(std::move(directionalLightSchema));
    }

    void ConsoleReflection::buildSkyboxSchema()
    {
        TargetSchema skyboxSchema;
        skyboxSchema.name = "Skybox";
        skyboxSchema.properties.push_back(PropertyAccessor{
            "horizonColor", "vec3",
            [](const void* object) { return formatVector3(static_cast<const SkyboxComponent*>(object)->horizonColor); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 horizonColor;
                if (!parseVector3(values, horizonColor, error))
                {
                    return false;
                }
                static_cast<SkyboxComponent*>(object)->horizonColor = horizonColor;
                return true;
            }
        });
        skyboxSchema.properties.push_back(PropertyAccessor{
            "zenithColor", "vec3",
            [](const void* object) { return formatVector3(static_cast<const SkyboxComponent*>(object)->zenithColor); },
            [](void* object, const std::vector<std::string>& values, std::string& error)
            {
                math::Vector3 zenithColor;
                if (!parseVector3(values, zenithColor, error))
                {
                    return false;
                }
                static_cast<SkyboxComponent*>(object)->zenithColor = zenithColor;
                return true;
            }
        });
        registry.registerSchema(std::move(skyboxSchema));
    }

    void ConsoleReflection::buildMeshSchema()
    {
        TargetSchema meshSchema;
        meshSchema.name = "Mesh";
        meshSchema.properties.push_back(PropertyAccessor{
            "path", "string",
            [](const void* object)
            {
                const MeshComponent* meshComponent = static_cast<const MeshComponent*>(object);
                if (!meshComponent->mesh)
                {
                    return std::string("(none)");
                }
                return meshComponent->mesh->sourcePath.value_or("(none)");
            },
            [](void*, const std::vector<std::string>&, std::string& error)
            {
                error = "Mesh.path is read-only";
                return false;
            }
        });
        registry.registerSchema(std::move(meshSchema));
    }

    void ConsoleReflection::registerCommands()
    {
        console->registerConsoleCommand("get", [this](const std::vector<std::string>& args, CommandContext& out)
        {
            handleGet(args, out);
        });
        console->registerConsoleCommand("set", [this](const std::vector<std::string>& args, CommandContext& out)
        {
            handleSet(args, out);
        });
        console->registerConsoleCommand("list", [this](const std::vector<std::string>& args, CommandContext& out)
        {
            handleList(args, out);
        });
    }

    void ConsoleReflection::handleGet(const std::vector<std::string>& args, CommandContext& out) const
    {
        if (args.empty())
        {
            out.write("Usage: get <address>");
            return;
        }

        const std::vector<std::string> address = splitAddress(args[0]);
        if (address.size() < 2 || address.size() > 3 || hasEmptySegment(address))
        {
            out.write("Invalid address: " + args[0]);
            return;
        }

        if (address[0] == CAMERA_TARGET_NAME && camera)
        {
            if (address.size() != 2)
            {
                out.write("Invalid address: " + args[0]);
                return;
            }

            const std::optional<std::string> value = readCameraProperty(*camera, address[1]);
            if (!value)
            {
                out.write("Unknown property: " + address[1]);
                return;
            }

            out.write(*value);
            return;
        }

        const std::optional<EntityId> id = resolveEntityId(address[0]);
        if (!id)
        {
            out.write("Unknown entity: " + address[0]);
            return;
        }

        if (address.size() == 2)
        {
            const TargetSchema* schema = registry.findSchema("Entity");
            const PropertyAccessor* property = schema->findProperty(address[1]);
            if (!property)
            {
                out.write("Unknown property: " + address[1]);
                return;
            }

            const Entity* entity = entityManager->getEntity(*id);
            out.write(property->read(entity));
            return;
        }

        const std::string& componentName = address[1];
        const TargetSchema* schema = registry.findSchema(componentName);
        if (!schema)
        {
            out.write("Unknown component: " + componentName);
            return;
        }

        const PropertyAccessor* property = schema->findProperty(address[2]);
        if (!property)
        {
            out.write("Unknown property: " + address[2]);
            return;
        }

        const void* componentPointer = resolveComponentPointer(*entityManager, *id, componentName);
        if (!componentPointer)
        {
            out.write(address[0] + " has no " + componentName + " component.");
            return;
        }

        out.write(property->read(componentPointer));
    }

    void ConsoleReflection::handleSet(const std::vector<std::string>& args, CommandContext& out) const
    {
        if (args.size() < 2)
        {
            out.write("Usage: set <address> <value...>");
            return;
        }

        const std::vector<std::string> address = splitAddress(args[0]);
        const std::vector<std::string> values(args.begin() + 1, args.end());
        if (address.size() < 2 || address.size() > 3 || hasEmptySegment(address))
        {
            out.write("Invalid address: " + args[0]);
            return;
        }

        if (address[0] == CAMERA_TARGET_NAME && camera)
        {
            if (address.size() != 2)
            {
                out.write("Invalid address: " + args[0]);
                return;
            }

            std::string error;
            if (!writeCameraProperty(*camera, address[1], values, error))
            {
                out.write("Failed to set " + args[0] + ": " + error);
                return;
            }

            out.write(args[0] + " set.");
            return;
        }

        const std::optional<EntityId> id = resolveEntityId(address[0]);
        if (!id)
        {
            out.write("Unknown entity: " + address[0]);
            return;
        }

        std::string error;

        if (address.size() == 2)
        {
            const TargetSchema* schema = registry.findSchema("Entity");
            const PropertyAccessor* property = schema->findProperty(address[1]);
            if (!property)
            {
                out.write("Unknown property: " + address[1]);
                return;
            }

            const Entity* entity = entityManager->getEntity(*id);
            Entity entityCopy = *entity;
            if (!property->write(&entityCopy, values, error))
            {
                out.write("Failed to set " + args[0] + ": " + error);
                return;
            }

            entityManager->setTransform(*id, entityCopy.transform);
            entityManager->setMobility(*id, entityCopy.mobility);
            if (entityCopy.name != entity->name)
            {
                entityManager->renameEntity(*id, entityCopy.name);
            }

            out.write(args[0] + " set.");
            return;
        }

        const std::string& componentName = address[1];
        const TargetSchema* schema = registry.findSchema(componentName);
        if (!schema)
        {
            out.write("Unknown component: " + componentName);
            return;
        }

        const PropertyAccessor* property = schema->findProperty(address[2]);
        if (!property)
        {
            out.write("Unknown property: " + address[2]);
            return;
        }

        if (componentName == "PointLight")
        {
            const PointLightComponent* component = entityManager->getPointLightComponent(*id);
            if (!component)
            {
                out.write(address[0] + " has no PointLight component.");
                return;
            }
            PointLightComponent componentCopy = *component;
            if (!property->write(&componentCopy, values, error))
            {
                out.write("Failed to set " + args[0] + ": " + error);
                return;
            }
            entityManager->addPointLightComponent(*id, componentCopy);
        }
        else if (componentName == "DirectionalLight")
        {
            const DirectionalLightComponent* component = entityManager->getDirectionalLightComponent();
            const Entity* directionalLightEntity = entityManager->getDirectionalLightEntity();
            if (!component || !directionalLightEntity || directionalLightEntity->id != *id)
            {
                out.write(address[0] + " has no DirectionalLight component.");
                return;
            }
            DirectionalLightComponent componentCopy = *component;
            if (!property->write(&componentCopy, values, error))
            {
                out.write("Failed to set " + args[0] + ": " + error);
                return;
            }
            entityManager->addDirectionalLightComponent(*id, componentCopy);
        }
        else if (componentName == "Skybox")
        {
            const SkyboxComponent* component = entityManager->getSkyboxComponent();
            const Entity* skyboxEntity = entityManager->getSkyboxEntity();
            if (!component || !skyboxEntity || skyboxEntity->id != *id)
            {
                out.write(address[0] + " has no Skybox component.");
                return;
            }
            SkyboxComponent componentCopy = *component;
            if (!property->write(&componentCopy, values, error))
            {
                out.write("Failed to set " + args[0] + ": " + error);
                return;
            }
            entityManager->addSkyboxComponent(*id, componentCopy);
        }
        else if (componentName == "Mesh")
        {
            out.write("Mesh." + address[2] + " is read-only.");
            return;
        }
        else
        {
            out.write("Unknown component: " + componentName);
            return;
        }

        out.write(args[0] + " set.");
    }

    void ConsoleReflection::handleList(const std::vector<std::string>& args, CommandContext& out) const
    {
        if (args.empty())
        {
            for (const Entity* entity : entityManager->getAllEntities())
            {
                out.write(entity->name + " (#" + std::to_string(entity->id) + ")");
            }
            return;
        }

        if (args[0] == CAMERA_TARGET_NAME && camera)
        {
            for (const std::string& propertyName : CAMERA_PROPERTY_NAMES)
            {
                const std::optional<std::string> value = readCameraProperty(*camera, propertyName);
                out.write(propertyName + " = " + value.value_or("<unavailable>"));
            }
            return;
        }

        const std::optional<EntityId> id = resolveEntityId(args[0]);
        if (!id)
        {
            out.write("Unknown entity: " + args[0]);
            return;
        }

        const Entity* entity = entityManager->getEntity(*id);
        const TargetSchema* entitySchema = registry.findSchema("Entity");
        for (const PropertyAccessor& property : entitySchema->properties)
        {
            out.write(property.name + " = " + property.read(entity));
        }

        for (const std::string& componentName : COMPONENT_SCHEMA_NAMES)
        {
            const void* componentPointer = resolveComponentPointer(*entityManager, *id, componentName);
            if (!componentPointer)
            {
                continue;
            }

            const TargetSchema* componentSchema = registry.findSchema(componentName);
            for (const PropertyAccessor& property : componentSchema->properties)
            {
                out.write(componentName + "." + property.name + " = " + property.read(componentPointer));
            }
        }
    }
}
