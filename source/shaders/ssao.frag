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

const int KERNEL_SIZE = 32;
const float RADIUS = 1.0;
const float BIAS = 0.025;

// Pseudo-random 2D hash
float hash(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// 4x4 dithered rotation angles to reduce banding
const float ditherPattern[16] = float[16](
    0.0,  8.0,  2.0, 10.0,
    12.0,  4.0, 14.0,  6.0,
    3.0, 11.0,  1.0,  9.0,
    15.0,  7.0, 13.0,  5.0
);

vec3 randomHemisphereDir(int index, vec2 uv, float rotAngle)
{
    float fi = float(index);
    float x = hash(uv + vec2(fi * 0.123, fi * 0.456)) * 2.0 - 1.0;
    float y = hash(uv + vec2(fi * 0.789, fi * 0.012)) * 2.0 - 1.0;
    float z = hash(uv + vec2(fi * 0.345, fi * 0.678));
    vec3 dir = normalize(vec3(x, y, z));

    // Apply per-pixel rotation to break up repeating patterns
    float cosR = cos(rotAngle);
    float sinR = sin(rotAngle);
    dir.xy = vec2(dir.x * cosR - dir.y * sinR, dir.x * sinR + dir.y * cosR);

    float scale = float(index) / float(KERNEL_SIZE);
    scale = mix(0.1, 1.0, scale * scale);
    return dir * scale;
}

vec3 reconstructViewPos(vec2 uv, float depth)
{
    mat4 invProj = inverse(globalUBO.proj);
    // Convert depth from Vulkan [0,1] range back to projection's [-1,1] clip space
    float clipZ = depth * 2.0 - 1.0;
    vec4 clipPos = vec4(uv * 2.0 - 1.0, clipZ, 1.0);
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

    // Reconstruct normal using best-fit finite differences (reduces staircase banding on curves)
    vec2 texelSize = 1.0 / textureSize(depthMap, 0);

    vec3 viewPosLeft  = reconstructViewPos(inUV + vec2(-texelSize.x, 0.0), texture(depthMap, inUV + vec2(-texelSize.x, 0.0)).r);
    vec3 viewPosRight = reconstructViewPos(inUV + vec2( texelSize.x, 0.0), texture(depthMap, inUV + vec2( texelSize.x, 0.0)).r);
    vec3 viewPosDown  = reconstructViewPos(inUV + vec2(0.0, -texelSize.y), texture(depthMap, inUV + vec2(0.0, -texelSize.y)).r);
    vec3 viewPosUp    = reconstructViewPos(inUV + vec2(0.0,  texelSize.y), texture(depthMap, inUV + vec2(0.0,  texelSize.y)).r);

    // Pick the derivative pair with the smallest depth difference (avoids edge artifacts)
    vec3 dx = (abs(viewPosRight.z - viewPos.z) < abs(viewPos.z - viewPosLeft.z))
        ? (viewPosRight - viewPos) : (viewPos - viewPosLeft);
    vec3 dy = (abs(viewPosUp.z - viewPos.z) < abs(viewPos.z - viewPosDown.z))
        ? (viewPosUp - viewPos) : (viewPos - viewPosDown);

    vec3 viewNormal = normalize(cross(dx, dy));

    // Create TBN from view-space normal
    vec3 randomVec = normalize(vec3(
        hash(inUV * 100.0) * 2.0 - 1.0,
        hash(inUV * 200.0) * 2.0 - 1.0,
        0.0));
    vec3 tangent = normalize(randomVec - viewNormal * dot(randomVec, viewNormal));
    vec3 bitangent = cross(viewNormal, tangent);
    mat3 TBN = mat3(tangent, bitangent, viewNormal);

    // Per-pixel dithered rotation from 4x4 Bayer pattern
    ivec2 pixelCoord = ivec2(gl_FragCoord.xy) % 4;
    float rotAngle = ditherPattern[pixelCoord.x + pixelCoord.y * 4] / 16.0 * 2.0 * 3.14159265;

    // Scale radius with distance: nearby geometry gets fine detail, far geometry gets broader AO
    float viewDistance = -viewPos.z;
    float scaledRadius = RADIUS * clamp(viewDistance * 0.1, 0.3, 3.0);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; i++)
    {
        vec3 sampleDir = TBN * randomHemisphereDir(i, inUV, rotAngle);
        vec3 samplePos = viewPos + sampleDir * scaledRadius;

        // Project sample to screen
        vec4 offset = globalUBO.proj * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xy = offset.xy * 0.5 + 0.5;

        float sampleDepth = texture(depthMap, offset.xy).r;
        vec3 sampleViewPos = reconstructViewPos(offset.xy, sampleDepth);

        // Range check and occlusion test
        float rangeCheck = smoothstep(0.0, 1.0, scaledRadius / abs(viewPos.z - sampleViewPos.z));
        occlusion += (sampleViewPos.z >= samplePos.z + BIAS ? 1.0 : 0.0) * rangeCheck;
    }

    outAO = 1.0 - (occlusion / float(KERNEL_SIZE));
}
