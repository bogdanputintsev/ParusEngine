#pragma once
#include <array>

#include "Core.h"

namespace tessera
{
        
    enum MouseButton
    {
        BUTTON_LEFT,
        BUTTON_RIGHT,
        BUTTON_MIDDLE,
    };

    inline const char* toString(MouseButton e)
    {
        switch (e)
        {
        case BUTTON_LEFT: return "BUTTON_LEFT";
        case BUTTON_RIGHT: return "BUTTON_RIGHT";
        case BUTTON_MIDDLE: return "BUTTON_MIDDLE";
        default: return "unknown";
        }
    }

    enum KeyButton
    {
        KEY_BACKSPACE = 0x08,
        KEY_ENTER = 0x0D,
        KEY_TAB = 0x09,
        KEY_SHIFT = 0x10,
        KEY_CONTROL = 0x11,

        KEY_PAUSE = 0x13,
        KEY_CAPITAL = 0x14,

        KEY_ESCAPE = 0x1B,

        KEY_CONVERT = 0x1C,
        KEY_NONCONVERT = 0x1D,
        KEY_ACCEPT = 0x1E,
        KEY_MODECHANGE = 0x1F,

        KEY_SPACE = 0x20,
        KEY_PRIOR = 0x21,
        KEY_NEXT = 0x22,
        KEY_END = 0x23,
        KEY_HOME = 0x24,
        KEY_LEFT = 0x25,
        KEY_UP = 0x26,
        KEY_RIGHT = 0x27,
        KEY_DOWN = 0x28,
        KEY_SELECT = 0x29,
        KEY_PRINT = 0x2A,
        KEY_SNAPSHOT = 0x2C,
        KEY_INSERT = 0x2D,
        KEY_DELETE = 0x2E,
        KEY_HELP = 0x2F,

        KEY_A = 0x41,
        KEY_B = 0x42,
        KEY_C = 0x43,
        KEY_D = 0x44,
        KEY_E = 0x45,
        KEY_F = 0x46,
        KEY_G = 0x47,
        KEY_H = 0x48,
        KEY_I = 0x49,
        KEY_J = 0x4A,
        KEY_K = 0x4B,
        KEY_L = 0x4C,
        KEY_M = 0x4D,
        KEY_N = 0x4E,
        KEY_O = 0x4F,
        KEY_P = 0x50,
        KEY_Q = 0x51,
        KEY_R = 0x52,
        KEY_S = 0x53,
        KEY_T = 0x54,
        KEY_U = 0x55,
        KEY_V = 0x56,
        KEY_W = 0x57,
        KEY_X = 0x58,
        KEY_Y = 0x59,
        KEY_Z = 0x5A,

        KEY_LWIN = 0x5B,
        KEY_RWIN = 0x5C,
        KEY_APPS = 0x5D,

        KEY_SLEEP = 0x5F,

        KEY_NUMPAD0 = 0x60,
        KEY_NUMPAD1 = 0x61,
        KEY_NUMPAD2 = 0x62,
        KEY_NUMPAD3 = 0x63,
        KEY_NUMPAD4 = 0x64,
        KEY_NUMPAD5 = 0x65,
        KEY_NUMPAD6 = 0x66,
        KEY_NUMPAD7 = 0x67,
        KEY_NUMPAD8 = 0x68,
        KEY_NUMPAD9 = 0x69,
        KEY_MULTIPLY = 0x6A,
        KEY_ADD = 0x6B,
        KEY_SEPARATOR = 0x6C,
        KEY_SUBTRACT = 0x6D,
        KEY_DECIMAL = 0x6E,
        KEY_DIVIDE = 0x6F,
        KEY_F1 = 0x70,
        KEY_F2 = 0x71,
        KEY_F3 = 0x72,
        KEY_F4 = 0x73,
        KEY_F5 = 0x74,
        KEY_F6 = 0x75,
        KEY_F7 = 0x76,
        KEY_F8 = 0x77,
        KEY_F9 = 0x78,
        KEY_F10 = 0x79,
        KEY_F11 = 0x7A,
        KEY_F12 = 0x7B,
        KEY_F13 = 0x7C,
        KEY_F14 = 0x7D,
        KEY_F15 = 0x7E,
        KEY_F16 = 0x7F,
        KEY_F17 = 0x80,
        KEY_F18 = 0x81,
        KEY_F19 = 0x82,
        KEY_F20 = 0x83,
        KEY_F21 = 0x84,
        KEY_F22 = 0x85,
        KEY_F23 = 0x86,
        KEY_F24 = 0x87,

        KEY_NUMLOCK = 0x90,
        KEY_SCROLL = 0x91,

        KEY_NUMPAD_EQUAL = 0x92,

        KEY_LSHIFT = 0xA0,
        KEY_RSHIFT = 0xA1,
        KEY_LCONTROL = 0xA2,
        KEY_RCONTROL = 0xA3,
        KEY_LMENU = 0xA4,
        KEY_RMENU = 0xA5,

        KEY_SEMICOLON = 0xBA,
        KEY_PLUS = 0xBB,
        KEY_COMMA = 0xBC,
        KEY_MINUS = 0xBD,
        KEY_PERIOD = 0xBE,
        KEY_SLASH = 0xBF,
        KEY_GRAVE = 0xC0,
    };

    inline const char* toString(KeyButton e)
    {
        switch (e)
        {
        case KEY_BACKSPACE: return "KEY_BACKSPACE";
        case KEY_ENTER: return "KEY_ENTER";
        case KEY_TAB: return "KEY_TAB";
        case KEY_SHIFT: return "KEY_SHIFT";
        case KEY_CONTROL: return "KEY_CONTROL";
        case KEY_PAUSE: return "KEY_PAUSE";
        case KEY_CAPITAL: return "KEY_CAPITAL";
        case KEY_ESCAPE: return "KEY_ESCAPE";
        case KEY_CONVERT: return "KEY_CONVERT";
        case KEY_NONCONVERT: return "KEY_NONCONVERT";
        case KEY_ACCEPT: return "KEY_ACCEPT";
        case KEY_MODECHANGE: return "KEY_MODECHANGE";
        case KEY_SPACE: return "KEY_SPACE";
        case KEY_PRIOR: return "KEY_PRIOR";
        case KEY_NEXT: return "KEY_NEXT";
        case KEY_END: return "KEY_END";
        case KEY_HOME: return "KEY_HOME";
        case KEY_LEFT: return "KEY_LEFT";
        case KEY_UP: return "KEY_UP";
        case KEY_RIGHT: return "KEY_RIGHT";
        case KEY_DOWN: return "KEY_DOWN";
        case KEY_SELECT: return "KEY_SELECT";
        case KEY_PRINT: return "KEY_PRINT";
        case KEY_SNAPSHOT: return "KEY_SNAPSHOT";
        case KEY_INSERT: return "KEY_INSERT";
        case KEY_DELETE: return "KEY_DELETE";
        case KEY_HELP: return "KEY_HELP";
        case KEY_A: return "KEY_A";
        case KEY_B: return "KEY_B";
        case KEY_C: return "KEY_C";
        case KEY_D: return "KEY_D";
        case KEY_E: return "KEY_E";
        case KEY_F: return "KEY_F";
        case KEY_G: return "KEY_G";
        case KEY_H: return "KEY_H";
        case KEY_I: return "KEY_I";
        case KEY_J: return "KEY_J";
        case KEY_K: return "KEY_K";
        case KEY_L: return "KEY_L";
        case KEY_M: return "KEY_M";
        case KEY_N: return "KEY_N";
        case KEY_O: return "KEY_O";
        case KEY_P: return "KEY_P";
        case KEY_Q: return "KEY_Q";
        case KEY_R: return "KEY_R";
        case KEY_S: return "KEY_S";
        case KEY_T: return "KEY_T";
        case KEY_U: return "KEY_U";
        case KEY_V: return "KEY_V";
        case KEY_W: return "KEY_W";
        case KEY_X: return "KEY_X";
        case KEY_Y: return "KEY_Y";
        case KEY_Z: return "KEY_Z";
        case KEY_LWIN: return "KEY_LWIN";
        case KEY_RWIN: return "KEY_RWIN";
        case KEY_APPS: return "KEY_APPS";
        case KEY_SLEEP: return "KEY_SLEEP";
        case KEY_NUMPAD0: return "KEY_NUMPAD0";
        case KEY_NUMPAD1: return "KEY_NUMPAD1";
        case KEY_NUMPAD2: return "KEY_NUMPAD2";
        case KEY_NUMPAD3: return "KEY_NUMPAD3";
        case KEY_NUMPAD4: return "KEY_NUMPAD4";
        case KEY_NUMPAD5: return "KEY_NUMPAD5";
        case KEY_NUMPAD6: return "KEY_NUMPAD6";
        case KEY_NUMPAD7: return "KEY_NUMPAD7";
        case KEY_NUMPAD8: return "KEY_NUMPAD8";
        case KEY_NUMPAD9: return "KEY_NUMPAD9";
        case KEY_MULTIPLY: return "KEY_MULTIPLY";
        case KEY_ADD: return "KEY_ADD";
        case KEY_SEPARATOR: return "KEY_SEPARATOR";
        case KEY_SUBTRACT: return "KEY_SUBTRACT";
        case KEY_DECIMAL: return "KEY_DECIMAL";
        case KEY_DIVIDE: return "KEY_DIVIDE";
        case KEY_F1: return "KEY_F1";
        case KEY_F2: return "KEY_F2";
        case KEY_F3: return "KEY_F3";
        case KEY_F4: return "KEY_F4";
        case KEY_F5: return "KEY_F5";
        case KEY_F6: return "KEY_F6";
        case KEY_F7: return "KEY_F7";
        case KEY_F8: return "KEY_F8";
        case KEY_F9: return "KEY_F9";
        case KEY_F10: return "KEY_F10";
        case KEY_F11: return "KEY_F11";
        case KEY_F12: return "KEY_F12";
        case KEY_F13: return "KEY_F13";
        case KEY_F14: return "KEY_F14";
        case KEY_F15: return "KEY_F15";
        case KEY_F16: return "KEY_F16";
        case KEY_F17: return "KEY_F17";
        case KEY_F18: return "KEY_F18";
        case KEY_F19: return "KEY_F19";
        case KEY_F20: return "KEY_F20";
        case KEY_F21: return "KEY_F21";
        case KEY_F22: return "KEY_F22";
        case KEY_F23: return "KEY_F23";
        case KEY_F24: return "KEY_F24";
        case KEY_NUMLOCK: return "KEY_NUMLOCK";
        case KEY_SCROLL: return "KEY_SCROLL";
        case KEY_NUMPAD_EQUAL: return "KEY_NUMPAD_EQUAL";
        case KEY_LSHIFT: return "KEY_LSHIFT";
        case KEY_RSHIFT: return "KEY_RSHIFT";
        case KEY_LCONTROL: return "KEY_LCONTROL";
        case KEY_RCONTROL: return "KEY_RCONTROL";
        case KEY_LMENU: return "KEY_LMENU";
        case KEY_RMENU: return "KEY_RMENU";
        case KEY_SEMICOLON: return "KEY_SEMICOLON";
        case KEY_PLUS: return "KEY_PLUS";
        case KEY_COMMA: return "KEY_COMMA";
        case KEY_MINUS: return "KEY_MINUS";
        case KEY_PERIOD: return "KEY_PERIOD";
        case KEY_SLASH: return "KEY_SLASH";
        case KEY_GRAVE: return "KEY_GRAVE";
        default: return "unknown";
        }
    }

    class Input final
    {
    public:
        Input() = default;
        void processKey(KeyButton key, const bool isPressed);
        void processButton(MouseButton button, const bool isPressed);
        void processMouseMove(const int newMouseX, const int newMouseY);
        static void processMouseWheel(int wheelDelta);

        [[nodiscard]] inline bool isKeyDown(KeyButton key) const;
        [[nodiscard]] inline bool isKeyUp(KeyButton key) const;
        [[nodiscard]] inline bool isMouseDown(MouseButton button) const;
        [[nodiscard]] inline bool isMouseUp(MouseButton button) const;
        [[nodiscard]] inline std::pair<int, int> getMousePosition() const;
    private:
        struct KeyboardState
        {
            std::array<bool, 256> keyPressed;
        }
        keyboardState{};

        struct MouseState
        {
            int mouseX;
            int mouseY;
            std::array<bool, 3> mousePressed;
        }
        mouseState{};
    };
        
}

