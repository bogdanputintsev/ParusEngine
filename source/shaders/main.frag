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

// Set 2: Material
layout(set = 2, binding = 0) uniform sampler2D albedoMap;
layout(set = 2, binding = 1) uniform sampler2D normalMap;
layout(set = 2, binding = 2) uniform sampler2D metallicMap;
layout(set = 2, binding = 3) uniform sampler2D roughnessMap;
layout(set = 2, binding = 4) uniform sampler2D aoMap;

// Set 3: Light sources
layout(set = 3, binding = 0, std140) uniform DirectionalLightUBO
{
    vec3 color;
    vec3 direction;
}   directionalLight;
layout(set = 3, binding = 1) uniform samplerCube environmentMap;
layout(set = 3, binding = 2) uniform sampler2DShadow shadowMap;
layout(set = 3, binding = 4) uniform sampler2D ssaoMap;

// Set 3: Point lights
struct PointLight
{
    vec4 positionAndRadius;
    vec4 colorAndIntensity;
};

layout(set = 3, binding = 3, std140) uniform PointLightUBO
{
    int count;
    int _plPad0;
    int _plPad1;
    int _plPad2;
    PointLight lights[4];
} pointLightUBO;

// =============================================
// Input / Output
// =============================================
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTextureCoordinate;
layout(location = 2) in mat3 inTBN;
layout(location = 5) in vec4 inLightSpacePos;

layout(location = 0) out vec4 outColor;
// =============================================
// Constants
// =============================================
const float AMBIENT_STRENGTH = 0.25;
const float PI = 3.1415926;
// =============================================
// Functions
// =============================================

//----------------------------------------------------------------------------------------------------------------------
// ACES Filmic Tone Mapping
//----------------------------------------------------------------------------------------------------------------------
vec3 ACESFilmic(vec3 color)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

//----------------------------------------------------------------------------------------------------------------------
// Normal Distribution Function (Trowbridge-Reitz GGX)
//----------------------------------------------------------------------------------------------------------------------
float NormalFunction (
    vec3 N,
    vec3 H,
    float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}

//----------------------------------------------------------------------------------------------------------------------
// Geometry function (Smith's Schlick-GGX)
//----------------------------------------------------------------------------------------------------------------------
float geometryFunctionOneDirectional(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}

float GeometryFunction(vec3 N, vec3 V, vec3 L, float k)
{
    float NdotV = max(dot(N, V), 0.0);
    float ggx1 = geometryFunctionOneDirectional(NdotV, k);

    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometryFunctionOneDirectional(NdotL, k);

    return ggx1 * ggx2;
}

//----------------------------------------------------------------------------------------------------------------------
// Fresnel function (Fresnel-Schlick)
//----------------------------------------------------------------------------------------------------------------------
vec3 FresnelFunction(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

//----------------------------------------------------------------------------------------------------------------------
// Shadow calculation with PCF
//----------------------------------------------------------------------------------------------------------------------
float calculateShadow(vec4 lightSpacePos, vec3 N, vec3 L)
{
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0)
    {
        return 1.0;
    }

    float bias = max(0.005 * (1.0 - dot(N, L)), 0.001);
    float currentDepth = projCoords.z - bias;

    // 5x5 PCF for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.5 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x)
    {
        for (int y = -2; y <= 2; ++y)
        {
            vec2 sampleCoord = projCoords.xy + vec2(x, y) * texelSize;
            shadow += texture(shadowMap, vec3(sampleCoord, currentDepth));
        }
    }
    shadow /= 25.0;

    return shadow;
}

//----------------------------------------------------------------------------------------------------------------------
// PBR BRDF calculation for a single light
//----------------------------------------------------------------------------------------------------------------------
vec3 calculateLight(vec3 L, vec3 radiance, vec3 N, vec3 V, vec3 F0, vec3 albedo, float metallic, float roughness)
{
    vec3 H = normalize(V + L);

    float NDF = NormalFunction(N, H, roughness);
    float G   = GeometryFunction(N, V, L, roughness);
    vec3 F    = FresnelFunction(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// =============================================
// Main Function
// =============================================
void main()
{
    // UV-based texture sampling
    vec3 albedo = texture(albedoMap, inTextureCoordinate).rgb;
    vec3 normalSample = texture(normalMap, inTextureCoordinate).rgb;
    vec3 normal = normalize(normalSample * 2.0 - 1.0);
    float metallic = texture(metallicMap, inTextureCoordinate).r;
    float roughness = texture(roughnessMap, inTextureCoordinate).r;
    float ao = texture(aoMap, inTextureCoordinate).r;

    if (globalUBO.debug == 1)
    {
        albedo = vec3(1.0);
    }

    vec3 N = normalize(inTBN * normal);
    vec3 V = normalize(globalUBO.cameraPos - inPosition);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // =========================================
    // Directional light (with shadow)
    // =========================================
    vec3 Lo = vec3(0.0);
    vec3 L = normalize(directionalLight.direction);
    vec3 radiance = directionalLight.color * 2.5;

    vec3 directionalContrib = calculateLight(L, radiance, N, V, F0, albedo, metallic, roughness);
    float shadow = calculateShadow(inLightSpacePos, N, L);
    Lo += directionalContrib * shadow;

    // =========================================
    // Point lights (no shadow, separate from directional shadow)
    // =========================================
    vec3 pointLightAmbient = vec3(0.0);

    for (int i = 0; i < pointLightUBO.count; ++i)
    {
        vec3 lightPos = pointLightUBO.lights[i].positionAndRadius.xyz;
        float lightRadius = pointLightUBO.lights[i].positionAndRadius.w;
        vec3 lightColor = pointLightUBO.lights[i].colorAndIntensity.xyz;
        float lightIntensity = pointLightUBO.lights[i].colorAndIntensity.w;

        vec3 lightDir = lightPos - inPosition;
        float distance = length(lightDir);

        if (distance < lightRadius)
        {
            vec3 pointL = normalize(lightDir);

            // Smooth windowed falloff (UE4-style)
            float distOverRadius = distance / lightRadius;
            float distOverRadius2 = distOverRadius * distOverRadius;
            float falloff = clamp(1.0 - distOverRadius2 * distOverRadius2, 0.0, 1.0);
            falloff *= falloff;
            float attenuation = falloff / (distance * distance + 1.0);
            vec3 pointRadiance = lightColor * lightIntensity * attenuation * 50.0;

            Lo += calculateLight(pointL, pointRadiance, N, V, F0, albedo, metallic, roughness);

            // Accumulate warm ambient contribution from point lights
            float ambientFalloff = 1.0 - distOverRadius * distOverRadius;
            pointLightAmbient += lightColor * lightIntensity * ambientFalloff * 0.15;
        }
    }

    // Ground-bounce fill light
    vec3 groundColor = vec3(0.1, 0.08, 0.06);
    float groundFactor = max(-N.y, 0.0) * 0.15;
    Lo += groundColor * albedo * groundFactor;

    // =========================================
    // Ambient lighting (blend sky + point light contribution)
    // =========================================
    vec3 envDiffuse = texture(environmentMap, N).rgb;

    // Blend sky ambient down when point light ambient is dominant (warm indoors)
    float skyAmbientWeight = clamp(1.0 - length(pointLightAmbient) * 2.0, 0.0, 1.0);
    vec2 screenUV = gl_FragCoord.xy / textureSize(ssaoMap, 0);
    float ssao = texture(ssaoMap, screenUV).r;
    vec3 ambient = (envDiffuse * skyAmbientWeight + pointLightAmbient) * albedo * ao * ssao * AMBIENT_STRENGTH;
    ambient = max(ambient, vec3(0.04) * albedo);
    vec3 color = ambient + Lo;

    // =========================================
    // Distance fog (applied in LINEAR space, before tone mapping)
    // =========================================
    float fogDistance = length(inPosition - globalUBO.cameraPos);
    float fogFactor = clamp((fogDistance - globalUBO.fogStart) / (globalUBO.fogEnd - globalUBO.fogStart), 0.0, 1.0);
    fogFactor = fogFactor * fogFactor;

    // Height-based fog: thicker near ground, thins at altitude
    float heightFog = exp(-max(inPosition.y, 0.0) * 0.015);
    fogFactor *= heightFog;

    color = mix(color, globalUBO.skyHorizonColor, fogFactor);

    // =========================================
    // Tone mapping (ACES Filmic) + Gamma correction
    // =========================================
    // Exposure adjustment before tone mapping
    color *= 1.4;

    color = ACESFilmic(color);
    // Note: gamma correction handled by VK_FORMAT_B8G8R8A8_SRGB swap chain

    outColor = vec4(color, 1.0);
}
