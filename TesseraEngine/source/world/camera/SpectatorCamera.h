#pragma once
#include <glm/vec3.hpp>

namespace tessera
{
    
    class SpectatorCamera
    {
    public:
        void updateTransform(float deltaTime);

        inline glm::vec3 getPosition() const { return position; }
        inline glm::vec3 getForwardVector() const { return forward; }
        inline glm::vec3 getUpVector() const { return up; }
        inline glm::vec3 getRightVector() const { return right; }
        
    private:
        glm::vec3 position = glm::vec3(65.8f, 55.4f, -45.32f);
        glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);

        float yaw = -90.0f;
        float pitch = 0.0f;
        float speed = 10.5f;
        float sensitivity = 0.25f;

        float speedAccelerationMultiplier = 3.5f;
    };
    
}
