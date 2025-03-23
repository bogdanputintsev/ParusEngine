#pragma once

#include "Math.h"

namespace tessera::math
{

    struct GlobalUbo
	{
        Matrix4x4 view; 
        Matrix4x4 projection;
    };

    struct InstanceUbo
    {
        Matrix4x4 model;
    };

}
