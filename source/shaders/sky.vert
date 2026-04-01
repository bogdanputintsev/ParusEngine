#version 450

// =============================================
// Descriptors
// =============================================
// Set 0: Global data
layout(set = 0, binding = 0, std140) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    mat4 lightSpaceMatrix;
    vec3 cameraPos;
    float _pad0;
    int debug;
    int _pad1;
    int _pad2;
    int _pad3;
    vec3 skyHorizonColor;
    float _pad4;
    vec3 skyZenithColor;
    float _pad5;
    float fogStart;
    float fogEnd;
    float time;
    float _timePad;
    vec3 sunDirection;
    float _sunDirPad;
} globalUBO;

// =============================================
// Input / Output
// =============================================
layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragDirection;

void main() {
    // Remove skybox position from matrix.
    mat4 viewRotationOnly = mat4(mat3(globalUBO.view));
    gl_Position = globalUBO.proj * viewRotationOnly * vec4(inPosition, 1.0);

    fragDirection = inPosition;
}
