#pragma once

#include <string>

#include "services/Service.h"

namespace parus
{

    /**
     * Abstract window/OS interface. Concrete implementations (e.g. PlatformWindows)
     * own the native handles and platform-specific message loop.
     */
    class Platform : public Service
    {
    public:
        virtual ~Platform() = default;

        virtual void init() = 0;
        virtual void clean() = 0;

        virtual void getMessages() = 0;
        virtual void processOnResize() const = 0;
        virtual void setWindowTitle(const std::string& title) = 0;

        /** Native window handle (HWND on Windows), for renderer surface creation and GUI backends. */
        [[nodiscard]] virtual void* getWindowHandle() const = 0;

        /** Native module/instance handle (HINSTANCE on Windows), for renderer surface creation. */
        [[nodiscard]] virtual void* getInstanceHandle() const = 0;
    };

}
