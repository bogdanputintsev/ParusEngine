#pragma once

#include "engine/Defines.h"
#include "services/Service.h"

#ifdef WITH_WINDOWS_PLATFORM
#define NOMINMAX
#include <windows.h>
#endif

namespace parus::imgui
{
    class ImGuiLibrary;
}

namespace parus
{
    
#ifdef WITH_WINDOWS_PLATFORM
    struct PlatformStorage
    {
    public:
        HINSTANCE hinstance;
        HWND hwnd;
        float clockFrequency;
        LARGE_INTEGER startTime;

        friend class parus::imgui::ImGuiLibrary;
    };
#endif
    
    class Platform final : public Service
    {
    public:
        void init();
        void clean();

        static void getMessages();
        void processOnResize() const;

        [[nodiscard]] PlatformStorage getPlatformStorage() const { return platformStorage; }
        
        friend class parus::imgui::ImGuiLibrary;
        
    private:
        PlatformStorage platformStorage;
        
    };
    
}


