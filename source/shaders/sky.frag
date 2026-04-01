#version 450

layout(set = 0, binding = 0, std140) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    vec3 cameraPos;
    float _pad0;
    int debug;
    int _pad1;
    int _pad2;
    int _pad3;
    vec3 skyHorizonColor;
    float _pad4;
    vec3 skyZenithColor;
} globalUBO;

layout(location = 0) in vec3 fragDirection;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 dir = normalize(fragDirection);
    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 color = mix(globalUBO.skyHorizonColor, globalUBO.skyZenithColor, t);
    outColor = vec4(color, 1.0);
}