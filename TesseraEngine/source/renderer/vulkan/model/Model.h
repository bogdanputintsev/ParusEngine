#pragma once
#include <memory>

#include "math/Math.h"

struct Mesh;

namespace tessera
{
        
    struct Model
    {
        std::shared_ptr<Mesh> mesh;
        math::Matrix4x4 transform = math::Matrix4x4::identity();
    };
        
}
