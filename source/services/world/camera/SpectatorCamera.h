#pragma once

#include "engine/utils/math/Math.h"

namespace parus
{
    
    class SpectatorCamera
    {
    public:
        void updateTransform(float deltaTime);

        [[nodiscard]] math::Vector3 getPosition() const { return position; }
        [[nodiscard]] math::Vector3 getForwardVector() const { return forward; }
        [[nodiscard]] math::Vector3 getUpVector() const { return up; }
        [[nodiscard]] math::Vector3 getRightVector() const { return right; }
        [[nodiscard]] float getYaw() const { return yaw; }
        [[nodiscard]] float getPitch() const { return pitch; }
        [[nodiscard]] float getSensitivity() const { return sensitivity; }
        [[nodiscard]] float getSpeedAccelerationMultiplier() const { return speedAccelerationMultiplier; }
        [[nodiscard]] float getSpeed() const { return speed; }

        void setPosition(const math::Vector3& newPosition) { position = newPosition; }
        void setYaw(const float newYaw) { yaw = newYaw; }
        void setPitch(const float newPitch) { pitch = newPitch; }
        void setSensitivity(const float newSensitivity) { sensitivity = newSensitivity; }
        void setSpeedAccelerationMultiplier(const float newMultiplier) { speedAccelerationMultiplier = newMultiplier; }
        void setSpeed(const float newSpeed) { speed = newSpeed; }
    private:
        math::Vector3 position = math::Vector3(65.8f, 55.4f, -45.32f);
        math::Vector3 forward = math::Vector3(0.0f, 0.0f, -1.0f);
        math::Vector3 up = math::Vector3(0.0f, 1.0f, 0.0f);
        math::Vector3 right = math::Vector3(1.0f, 0.0f, 0.0f);

        float yaw = -90.0f;
        float pitch = 0.0f;
        float speed = 20.5f;
        float sensitivity = 0.25f;

        float speedAccelerationMultiplier = 3.5f;
    };
    
}
