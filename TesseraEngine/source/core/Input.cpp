#include "Input.h"

namespace tessera
{

    void Input::processKey(const KeyButton key, const bool isPressed)
    {
        if (keyboardState.keyPressed[key] != isPressed)
        {
            keyboardState.keyPressed[key] = isPressed;
            
            FIRE_EVENT(isPressed
                ? EventType::EVENT_KEY_PRESSED
                : EventType::EVENT_KEY_RELEASED,
                key);
            
            LOG_DEBUG("Key " + std::string(toString(key)) + " is " + (isPressed ? "pressed" : "released"));
        }
    }
    
    void Input::processButton(const MouseButton button, const bool isPressed)
    {
        if (mouseState.mousePressed[button] != isPressed)
        {
            mouseState.mousePressed[button] = isPressed;

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
        
    bool Input::isKeyDown(const KeyButton key) const
    {
        return keyboardState.keyPressed[key] == true;
    }

    bool Input::isKeyUp(const KeyButton key) const
    {
        return keyboardState.keyPressed[key] == false;
    }

    bool Input::isMouseDown(const MouseButton button) const
    {
        return mouseState.mousePressed[button] == true;
    }

    bool Input::isMouseUp(const MouseButton button) const
    {
        return mouseState.mousePressed[button] == false;
    }

    std::pair<int, int> Input::getMousePosition() const
    {
        return { mouseState.mouseX, mouseState.mouseY };
    }

}
