#include "Input.h"

namespace tessera
{

    void Input::processKey(const KeyButton key, const bool isPressed)
    {
        const int keyCode = static_cast<int>(key);
        
        if (keyboardState.keyPressed[keyCode] != isPressed)
        {
            keyboardState.keyPressed[keyCode] = isPressed;
            
            FIRE_EVENT(isPressed
                ? EventType::EVENT_KEY_PRESSED
                : EventType::EVENT_KEY_RELEASED,
                key);
            
            LOG_DEBUG("Key " + std::string(toString(key)) + " is " + (isPressed ? "pressed" : "released"));
        }
    }

    void Input::processChar(const char inputChar)
    {
        FIRE_EVENT(tessera::EventType::EVENT_CHAR_INPUT, inputChar);
        LOG_DEBUG("Char entered: " + std::string(1, inputChar));
    }

    void Input::processButton(const MouseButton button, const bool isPressed)
    {
        const int buttonCode = static_cast<int>(button);

        if (mouseState.mousePressed[buttonCode] != isPressed)
        {
            mouseState.mousePressed[buttonCode] = isPressed;

            FIRE_EVENT(isPressed
                ? EventType::EVENT_MOUSE_BUTTON_PRESSED
                : EventType::EVENT_MOUSE_BUTTON_RELEASED,
                button);
        
            LOG_DEBUG("Mouse " + std::string(toString(button)) + " is " + (isPressed ? "pressed" : "released"));
        }
    }

    void Input::processMouseMove(const int newMouseX, const int newMouseY)
    {
        if (mouseState.mouseX != newMouseX || mouseState.mouseY != newMouseY)
        {
            // LOG_DEBUG("Mouse position: " + std::to_string(newMouseX) + ", " + std::to_string(newMouseY));

            mouseState.mouseX = newMouseX;
            mouseState.mouseY = newMouseY;

            FIRE_EVENT(EventType::EVENT_MOUSE_MOVED, newMouseX, newMouseY);
        }
    }

    void Input::processMouseWheel(const int wheelDelta)
    {
        FIRE_EVENT(EventType::EVENT_MOUSE_WHEEL, wheelDelta);
    }
}
