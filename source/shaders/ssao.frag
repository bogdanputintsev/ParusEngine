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

layout(set = 1, binding = 0) uniform sampler2D depthMap;

layout(location = 0) in vec2 inUV;
layout(location = 0) out float outAO;

const int KERNEL_SIZE = 16;
const float RADIUS = 0.5;
const float BIAS = 0.025;

// Pseudo-random 2D hash
float hash(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 randomHemisphereDir(int index, vec2 uv)
{
    float fi = float(index);
    float x = hash(uv + vec2(fi * 0.123, fi * 0.456)) * 2.0 - 1.0;
    float y = hash(uv + vec2(fi * 0.789, fi * 0.012)) * 2.0 - 1.0;
    float z = hash(uv + vec2(fi * 0.345, fi * 0.678));
    vec3 dir = normalize(vec3(x, y, z));
    float scale = float(index) / float(KERNEL_SIZE);
    scale = mix(0.1, 1.0, scale * scale);
    return dir * scale;
}

vec3 reconstructViewPos(vec2 uv, float depth)
{
    mat4 invProj = inverse(globalUBO.proj);
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth, 1.0);
    vec4 viewPos = invProj * clipPos;
    return viewPos.xyz / viewPos.w;
}

void main()
{
    float depth = texture(depthMap, inUV).r;

    // Skip sky pixels (depth = 1.0)
    if (depth >= 1.0)
    {
        outAO = 1.0;
        return;
    }

    vec3 viewPos = reconstructViewPos(inUV, depth);

    // Reconstruct normal from depth buffer using cross product of partial derivatives
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);
    float depthRight = texture(depthMap, inUV + vec2(texelSize.x, 0.0)).r;
    float depthUp = texture(depthMap, inUV + vec2(0.0, texelSize.y)).r;
    vec3 viewPosRight = reconstructViewPos(inUV + vec2(texelSize.x, 0.0), depthRight);
    vec3 viewPosUp = reconstructViewPos(inUV + vec2(0.0, texelSize.y), depthUp);
    vec3 viewNormal = normalize(cross(viewPosRight - viewPos, viewPosUp - viewPos));

    // Create TBN from view-space normal
    vec3 randomVec = normalize(vec3(
        hash(inUV * 100.0) * 2.0 - 1.0,
        hash(inUV * 200.0) * 2.0 - 1.0,
        0.0));
    vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, viewNormal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        vec3 sampleDir = TBN * randomHemisphereDir(i, inUV);
        vec3 samplePos = viewPos + sampleDir * RADIUS;

        // Project sample to screen
        vec4 offset = globalUBO.proj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(depthMap, offset.xy).r;
        vec3 sampleViewPos = reconstructViewPos(offset.xy, sampleDepth);

        // Range check and occlusion test
        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(viewPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + BIAS ? 1.0 : 0.0) * rangeCheck;
    }

    outAO = 1.0 - (occlusion / float(KERNEL_SIZE));
}
