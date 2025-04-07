#pragma once

#include "Math.h"

namespace tessera::math
{

    struct GlobalUbo
	{
        alignas(16) Matrix4x4 view; 
        alignas(16) Matrix4x4 projection;
        alignas(16) Matrix4x4 cameraPosition;
        bool debug = false;
    };

    struct InstanceUbo
    {
        alignas(16) Matrix4x4 model;
        alignas(16) Matrix4x4 normal;
    };

    struct DirectionalLightUbo
    {
        TrivialVector3 color;
        TrivialVector3 direction;
    };
    
    // struct LightUbo
    // {
    //     int lightCount;
    //     TrivialVector3 color[vulkan::MAX_NUMBER_OF_LIGHTS];
    //     float intensity[vulkan::MAX_NUMBER_OF_LIGHTS];
    //     vulkan::LightType type[vulkan::MAX_NUMBER_OF_LIGHTS];
    //
    //     // Directional light only
    //     TrivialVector3 direction[vulkan::MAX_NUMBER_OF_LIGHTS];
    //
    //     // Point light only
    //     TrivialVector3 position[vulkan::MAX_NUMBER_OF_LIGHTS];
    //     float radius[vulkan::MAX_NUMBER_OF_LIGHTS];
    // };
    
}
