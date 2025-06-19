#pragma once
#include <glm/glm.hpp>

namespace tessera
{

    struct GlobalUbo
	{
        glm::mat4 view;
        glm::mat4 proj;
    };

    struct InstanceUbo
    {
        glm::mat4 model;
    };

}
