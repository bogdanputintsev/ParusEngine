#include "Platform.h"
#include "core/Defines.h"

#ifdef WITH_WINDOWS_PLATFORM

#include <windows.h>
#include <windowsx.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#include "Core.h"
#include "Event.h"
#include "core/Input.h"
#include "utils/TesseraLog.h"

namespace tessera
{
    
    struct WindowsPlatformState final : PlatformState
    {
    public:
        HINSTANCE hinstance;
        HWND hwnd;
        float clockFrequency;
        LARGE_INTEGER startTime;
    };
    
    LRESULT CALLBACK win32ProcessMessage(HWND hwnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
            case WM_ERASEBKGND:
                // Notify the OS that erasing will be handled by the application to prevent flicker.
                return 1;
            case WM_CLOSE:
                FIRE_EVENT(tessera::EventType::EVENT_APPLICATION_QUIT, 0);
                return TRUE;
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_SIZE: {
                RECT r;
                GetClientRect(hwnd, &r);
                const int width = r.right - r.left;
                const int height = r.bottom - r.top;
                FIRE_EVENT(tessera::EventType::EVENT_WINDOW_RESIZED, width, height);
            } break;
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_SYSKEYUP:
            {
                const bool isPressed = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
                const auto keyButton = static_cast<KeyButton>(wParam);
                CORE->inputSystem->processKey(keyButton, isPressed);
            } break;
            case WM_MOUSEMOVE:
            {
                const int mouseX = GET_X_LPARAM(lParam);
                const int mouseY = GET_Y_LPARAM(lParam);
                CORE->inputSystem->processMouseMove(mouseX, mouseY);
            } break;
            case WM_MOUSEWHEEL:
            {
                int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
                if (zDelta != 0)
                {
                    zDelta = (zDelta < 0) ? -1 : 1;
                    CORE->inputSystem->processMouseWheel(zDelta);
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
                        mouseButton = BUTTON_LEFT;
                        break;
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                        mouseButton = BUTTON_MIDDLE;
                        break;
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                        mouseButton = BUTTON_RIGHT;
                        break;
                    default: ;
                }

                // Pass over to the input subsystem.
                if (mouseButton.has_value())
                {
                    CORE->inputSystem->processButton(mouseButton.value(), isPressed);
                }
            } break;
            default: ;
        }

        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    
    void Platform::init(const char* applicationName, int posX, int posY, int width, int height)
    {
        platformState = std::make_unique<WindowsPlatformState>();
        const auto state = dynamic_cast<WindowsPlatformState*>(platformState.get());
        state->hinstance = GetModuleHandleA(nullptr);

        HICON icon = LoadIcon(state->hinstance, IDI_APPLICATION);
        WNDCLASSA wc = {};
        wc.style = CS_DBLCLKS;  // Get double-clicks
        wc.lpfnWndProc = win32ProcessMessage;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 0;
        wc.hInstance = state->hinstance;
        wc.hIcon = icon;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);  // NULL; // Manage the cursor manually
        wc.hbrBackground = nullptr;                   // Transparent
        wc.lpszClassName = "tessera_window_class";

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

        // In this case, the border rectangle is negative.
        posX += borderRect.left;
        posY += borderRect.top;

        // Grow by the size of the OS border.
        width += borderRect.right - borderRect.left;
        height += borderRect.bottom - borderRect.top;

        HWND handle = CreateWindowExA(
            windowExStyle, "tessera_window_class", applicationName,
            windowStyle, posX, posY, width, height,
            nullptr, nullptr, state->hinstance, nullptr);

        ASSERT(handle, "Window creation failed!");
        state->hwnd = handle;
        
        // If initially minimized, use SW_MINIMIZE : SW_SHOWMINNOACTIVE;
        // If initially maximized, use SW_SHOWMAXIMIZED : SW_MAXIMIZE
        ShowWindow(state->hwnd, SW_SHOW);

        // Clock setup
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        state->clockFrequency = 1.0f / static_cast<float>(frequency.QuadPart);
        QueryPerformanceCounter(&state->startTime);
    }

    VkSurfaceKHR Platform::createVulkanSurface(const VkInstance instance)
    {
        ASSERT(platformState.get(), "Platform state is unavailable.");
        const auto state = dynamic_cast<WindowsPlatformState*>(platformState.get());

        VkWin32SurfaceCreateInfoKHR createInfo = {VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR};
        createInfo.hinstance = state->hinstance;
        createInfo.hwnd = state->hwnd;

        VkSurfaceKHR surface;
        ASSERT(vkCreateWin32SurfaceKHR(instance, &createInfo, nullptr, &surface) == VK_SUCCESS,
            "Vulkan surface creation failed");

        return surface;
    }

    void Platform::clean()
    {
        if (platformState)
        {
            const auto state = dynamic_cast<WindowsPlatformState*>(platformState.get());
            if (state->hwnd)
            {
                DestroyWindow(state->hwnd);
                state->hwnd = nullptr;
            }
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
}
#endif

