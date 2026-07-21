#pragma once

#include "Platform.h"
#include "engine/Defines.h"

#ifdef WITH_WINDOWS_PLATFORM

#define NOMINMAX
#include <windows.h>

namespace parus
{

    struct PlatformStorage
    {
        HINSTANCE hinstance;
        HWND hwnd;
        float clockFrequency;
        LARGE_INTEGER startTime;
    };

    class PlatformWindows final : public Platform
    {
    public:
        void init() override;
        void clean() override;

        void getMessages() override;
        void processOnResize() const override;
        void setWindowTitle(const std::string& title) override;

        [[nodiscard]] void* getWindowHandle() const override { return platformStorage.hwnd; }
        [[nodiscard]] void* getInstanceHandle() const override { return platformStorage.hinstance; }

    private:
        PlatformStorage platformStorage{};

    };

}

#endif
