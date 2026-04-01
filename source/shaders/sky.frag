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

layout(location = 0) in vec3 fragDirection;
layout(location = 0) out vec4 outColor;

// Hash-based noise (no texture needed)
float hash(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    for (int i = 0; i < 5; i++)
    {
        value += amplitude * valueNoise(p * frequency);
        frequency *= 2.0;
        amplitude *= 0.5;
    }

    return value;
}

void main()
{
    vec3 dir = normalize(fragDirection);
    vec3 sunDir = normalize(globalUBO.sunDirection);

    // Atmosphere gradient
    float elevation = max(dir.y, 0.0);
    float t = pow(elevation, 0.4);
    vec3 skyColor = mix(globalUBO.skyHorizonColor, globalUBO.skyZenithColor, t);

    // Horizon haze (slight brightening near horizon)
    float horizonHaze = exp(-elevation * 4.0) * 0.3;
    skyColor += vec3(horizonHaze);

    // Sun disk
    float sunAngle = dot(dir, sunDir);
    float sunDisk = smoothstep(0.9995, 0.9999, sunAngle);
    vec3 sunColor = vec3(1.4, 1.2, 0.9);
    skyColor += sunDisk * sunColor * 2.0;

    // Sun glow (larger soft halo)
    float sunGlow = pow(max(sunAngle, 0.0), 256.0) * 1.5;
    skyColor += sunGlow * vec3(1.0, 0.8, 0.4);

    // Medium sun glow
    float sunGlowMed = pow(max(sunAngle, 0.0), 32.0) * 0.3;
    skyColor += sunGlowMed * vec3(1.0, 0.6, 0.3);

    // Clouds (only above horizon)
    if (dir.y > 0.01)
    {
        // Project direction onto a flat cloud plane
        vec2 cloudUV = dir.xz / (dir.y + 0.1) * 2.0;
        float slowTime = globalUBO.time * 0.02;
        cloudUV += vec2(slowTime, slowTime * 0.3);

        float clouds = fbm(cloudUV * 1.5);
        clouds = smoothstep(0.35, 0.65, clouds);

        // Fade clouds near horizon to avoid hard cutoff
        float cloudFade = smoothstep(0.01, 0.15, dir.y);
        clouds *= cloudFade;

        // Cloud lighting: brighter on sun side
        float cloudLight = dot(sunDir, vec3(0.0, 1.0, 0.0)) * 0.3 + 0.7;
        vec3 cloudColor = mix(vec3(0.6, 0.6, 0.65), vec3(1.0, 0.98, 0.95), cloudLight);

        skyColor = mix(skyColor, cloudColor, clouds * 0.7);
    }

    // Below horizon: darken
    if (dir.y < 0.0)
    {
        float below = clamp(-dir.y * 5.0, 0.0, 1.0);
        skyColor = mix(skyColor, globalUBO.skyHorizonColor * 0.5, below);
    }

    outColor = vec4(skyColor, 1.0);
}
