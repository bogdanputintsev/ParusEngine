#version 450

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

layout(set = 1, binding = 0, std140) uniform InstanceUBO {
    mat4 model;
    mat4 normalMatrix;
} instanceUBO;

layout(location = 0) in vec3 inPosition;

void main()
{
    gl_Position = globalUBO.proj * globalUBO.view * instanceUBO.model * vec4(inPosition, 1.0);
}
