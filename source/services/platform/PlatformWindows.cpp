﻿#include "Platform.h"
#include "engine/Defines.h"

#ifdef WITH_WINDOWS_PLATFORM

#include <optional>
#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_win32.h>

#include "engine/Event.h"
#include "engine/EngineCore.h"
#include "engine/input/Input.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus
{
    
    static LRESULT CALLBACK win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
            case WM_ERASEBKGND:
                // Notify the OS that erasing will be handled by the application to prevent flicker.
                return 1;
            case WM_CLOSE:
                FIRE_EVENT(parus::EventType::EVENT_APPLICATION_QUIT, 0);
                return TRUE;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_SIZE: {
                Services::get<Platform>()->processOnResize();
            } break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const bool isPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                const auto keyButton = static_cast<KeyButton>(wParam);
                Services::get<Input>()->processKey(keyButton, isPressed);
            } break;
            case WM_MOUSEMOVE:
            {
                const int mouseX = GET_X_LPARAM(lParam);
                const int mouseY = GET_Y_LPARAM(lParam);
                Services::get<Input>()->processMouseMove(mouseX, mouseY);
            } break;
            case WM_MOUSEWHEEL:
            {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                if (zDelta != 0)
                {
                    zDelta = (zDelta < 0) ? -1 : 1;
                    Services::get<Input>()->processMouseWheel(zDelta);
                }
            } break;
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            {
                const bool isPressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
                std::optional<MouseButton> mouseButton;
                    
                switch (msg)
                {
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                        mouseButton = MouseButton::BUTTON_LEFT;
                        break;
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                        mouseButton = MouseButton::BUTTON_MIDDLE;
                        break;
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                        mouseButton = MouseButton::BUTTON_RIGHT;
                        break;
                    default: ;
                }

                if (mouseButton.has_value())
                {
                    Services::get<Input>()->processButton(mouseButton.value(), isPressed);
                }
            } break;
            case WM_CHAR:
            {
                const char c = static_cast<char>(wParam);
                Services::get<Input>()->processChar(c);
            } break;

            default: ;
        }

        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    
    void Platform::init()
    {
        platformState.hinstance = GetModuleHandleA(nullptr);

        const HICON icon = LoadIcon(platformState.hinstance, IDI_APPLICATION);
        WNDCLASSA wc = {};
        wc.style = CS_DBLCLKS;  // Get double-clicks
        wc.lpfnWndProc = win32ProcessMessage;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = platformState.hinstance;
        wc.hIcon = icon;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);  // NULL; // Manage the cursor manually
        wc.hbrBackground = nullptr;                   // Transparent
        wc.lpszClassName = "parus_window_class";

        ASSERT(RegisterClassA(&wc), "Failed to init platform.");

        // Create window
        uint32_t windowStyle = WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION;
        constexpr uint32_t windowExStyle = WS_EX_APPWINDOW;

        windowStyle |= WS_MAXIMIZEBOX;
        windowStyle |= WS_MINIMIZEBOX;
        windowStyle |= WS_THICKFRAME;

        // Obtain the size of the border.
        RECT borderRect = {0, 0, 0, 0};
        AdjustWindowRectEx(&borderRect, windowStyle, 0, windowExStyle);

        const std::string title = Services::get<Configs>()->get("Window", "title");
        
		int positionX = Services::get<Configs>()->getAsInt("Window", "positionX").value_or(0);
		int positionY = Services::get<Configs>()->getAsInt("Window", "positionY").value_or(0);
        int width = Services::get<Configs>()->getAsInt("Window", "width").value_or(0);
        int height = Services::get<Configs>()->getAsInt("Window", "height").value_or(0);
        
        // In this case, the border rectangle is negative.
        positionX += borderRect.left;
        positionY += borderRect.top;
        Services::get<Configs>()->write("Window", "positionX", std::to_string(positionX));
        Services::get<Configs>()->write("Window", "positionY", std::to_string(positionY));

        // Grow by the size of the OS border.
        width += borderRect.right - borderRect.left;
        height += borderRect.bottom - borderRect.top;
        Services::get<Configs>()->write("Window", "width", std::to_string(width));
        Services::get<Configs>()->write("Window", "height", std::to_string(height));

        const HWND handle = CreateWindowExA(
            windowExStyle, "parus_window_class", title.c_str(),
            windowStyle, positionX, positionY, width, height,
            nullptr, nullptr, platformState.hinstance, nullptr);

        ASSERT(handle, "Window creation failed!");
        platformState.hwnd = handle;
        
        // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
        // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
        ShowWindow(platformState.hwnd, SW_SHOW);

        // Clock setup
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        platformState.clockFrequency = 1.0f / static_cast<float>(frequency.QuadPart);
        QueryPerformanceCounter(&platformState.startTime);
    }

    void Platform::createVulkanSurface(vulkan::VulkanStorage& vulkanStorage) const
    {
        VkWin32SurfaceCreateInfoKHR createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = platformState.hinstance;
        createInfo.hwnd = platformState.hwnd;
        createInfo.pNext = nullptr;

        ASSERT(vkCreateWin32SurfaceKHR(vulkanStorage.instance, &createInfo, nullptr, &vulkanStorage.surface) == VK_SUCCESS,
            "Vulkan surface creation failed");
    }

    void Platform::clean()
    {
        if (platformState.hwnd)
        {
            DestroyWindow(platformState.hwnd);
            platformState.hwnd = nullptr;
        }
    }

    void Platform::getMessages()
    {
        MSG message;
        while (PeekMessageA(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }

    void Platform::processOnResize() const
    {
        RECT r;
        GetClientRect(platformState.hwnd, &r);
        const int newWidth = r.right - r.left;
        const int newHeight = r.bottom - r.top;

        if (newWidth == 0 || newHeight == 0)
        {
            Services::get<Configs>()->write("Window", "isMinimized", "true");
            LOG_INFO("Window has been minimized.");
            FIRE_EVENT(parus::EventType::EVENT_WINDOW_MINIMIZED, true);
            return;
        }

		const bool isMinimized = Services::get<Configs>()->getAsBool("Window", "isMinimized").value_or(false);
        if (isMinimized)
        {
            Services::get<Configs>()->write("Window", "isMinimized", "false");
            LOG_INFO("Window has been restored.");
            FIRE_EVENT(parus::EventType::EVENT_WINDOW_MINIMIZED, false);
        }

        Services::get<Configs>()->write("Window", "width", std::to_string(newWidth));
        Services::get<Configs>()->write("Window", "height", std::to_string(newWidth));
        
        FIRE_EVENT(parus::EventType::EVENT_WINDOW_RESIZED, newWidth, newHeight);
    }

}
#endif

