#pragma once
#include <string>

#include "core/Defines.h"
#include "Utils/Utils.h"

namespace tessera
{
    
    struct ApplicationInfo
    {
    public:
        struct GeneralInfo
        {
            std::string applicationName = "Tessera Engine";
            u8 versionMajor = 0;
            u8 versionMinor = 3;
            u8 versionPatch = 0;
        } generalInfo;
        
        struct WindowInfo
        {
            std::string windowTitle = "Tessera Engine";
            u32 positionX = 250;
            u32 positionY = 250;
            u32 width = 1200;
            u32 height = 900;
            bool isMinimized = false;
        } windowInfo;

        void readAll();
        void saveAll() const;

    private:
        static constexpr auto APPLICATION_INFO_FILE_PATH = "bin/config/tessera.about";
        
    };
    
}
