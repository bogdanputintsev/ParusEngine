#version 450

// Global UBO: view and projection matrices
layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

// Instance UBO: model matrix for each object
layout(set = 0, binding = 1) uniform InstanceUBO {
    mat4 model;
} instanceUBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    // Apply the model transformation first, then view and projection.
    gl_Position = globalUBO.proj * globalUBO.view * instanceUBO.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}