#pragma once

#include "Math.h"

namespace parus::math
{

    struct alignas(16) GlobalUbo
    {
        alignas(16) TrivialMatrix4x4 view;
        alignas(16) TrivialMatrix4x4 projection;
        alignas(16) TrivialMatrix4x4 lightSpaceMatrix;
        TrivialVector3 cameraPosition;
        int debug = 0;
        TrivialVector3 skyHorizonColor;
        TrivialVector3 skyZenithColor;
        float fogStart = 200.0f;
        float fogEnd = 1200.0f;
        float time = 0.0f;
        float _timePad = 0.0f;
        TrivialVector3 sunDirection;
    };

    struct alignas(16) InstanceUbo
    {
        alignas(16) TrivialMatrix4x4 model;
        alignas(16) TrivialMatrix4x4 normal;
    };

    struct alignas(16) DirectionalLightUbo
    {
        TrivialVector3 color;
        TrivialVector3 direction;
    };

    static constexpr int MAX_POINT_LIGHTS = 4;

    struct alignas(16) PointLightEntry
    {
        float posX = 0.0f;
        float posY = 0.0f;
        float posZ = 0.0f;
        float radius = 0.0f;
        float colorR = 0.0f;
        float colorG = 0.0f;
        float colorB = 0.0f;
        float intensity = 0.0f;
    };

    struct alignas(16) PointLightUbo
    {
        int count = 0;
        int _pad0 = 0;
        int _pad1 = 0;
        int _pad2 = 0;
        PointLightEntry lights[MAX_POINT_LIGHTS];
    };

}
