#pragma once
#include <glm/glm.hpp>

namespace tessera
{

    struct UniformBufferObject
	{
        glm::mat4 model;
        glm::mat4 view;
        glm::mat4 proj;
    };

}
