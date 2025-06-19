#include "SpectatorCamera.h"

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "core/Input.h"

namespace tessera
{
    void SpectatorCamera::updateTransform(const float deltaTime)
    {
        const auto input = CORE->inputSystem;
        
        // Process keyboard
        float velocity = speed * deltaTime;
        if (input->isKeyPressed(KeyButton::KEY_SHIFT))
        {
            velocity *= speedAccelerationMultiplier;
        }
        if (input->isKeyPressed(KeyButton::KEY_W))
        {
            position += forward * velocity;
        }
        if (input->isKeyPressed(KeyButton::KEY_S))
        {
            position -= forward * velocity;
        }
        if (input->isKeyPressed(KeyButton::KEY_A))
        {
            position -= right * velocity;
        }
        if (input->isKeyPressed(KeyButton::KEY_D))
        {
            position += right * velocity;
        }
        if (input->isKeyPressed(KeyButton::KEY_E))
        {
            position += up * velocity;
        }
        if (input->isKeyPressed(KeyButton::KEY_Q))
        {
            position -= up * velocity;
        }
        
        // Process mouse.
        if (input->isMouseDown(MouseButton::BUTTON_RIGHT))
        {
            auto [mouseOffsetX, mouseOffsetY] = input->getMouseOffset();
            const float offsetX = static_cast<float>(mouseOffsetX) * sensitivity;
            const float offsetY = static_cast<float>(mouseOffsetY) * sensitivity;

            yaw += offsetX;
            pitch = std::clamp(pitch + offsetY, -90.0f, 90.0f);
        }
        
        // Calculate new direction vector.
        glm::vec3 direction;
        direction.x = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = -cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        forward = glm::normalize(direction);

        // Recalculate right and up vectors.
        right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        up = glm::normalize(glm::cross(right, forward));

    }
    
}
