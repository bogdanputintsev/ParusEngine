#version 450

layout(set = 0, binding = 0) uniform sampler2D ssaoInput;
layout(set = 0, binding = 1) uniform sampler2D depthMap;

layout(location = 0) in vec2 inUV;
layout(location = 0) out float outAO;

void main()
{
    vec2 texelSize = 1.0 / textureSize(ssaoInput, 0);
    float centerAO = texture(ssaoInput, inUV).r;
    float centerDepth = texture(depthMap, inUV).r;

    // Skip sky
    if (centerDepth >= 1.0)
    {
        outAO = 1.0;
        return;
    }

    // Bilateral blur: two-pass style 9x9 Gaussian-weighted, depth-aware
    // Gaussian weights for sigma ~3.0 (precomputed for offsets 0..4)
    const float gaussWeights[5] = float[5](0.2270, 0.1946, 0.1216, 0.0541, 0.0162);
    const float depthThreshold = 0.02;

    float totalWeight = 0.0;
    float totalAO = 0.0;

    for (int x = -4; x <= 4; x++)
    {
        for (int y = -4; y <= 4; y++)
        {
            vec2 sampleUV = inUV + vec2(float(x), float(y)) * texelSize;
            float sampleAO = texture(ssaoInput, sampleUV).r;
            float sampleDepth = texture(depthMap, sampleUV).r;

            // Gaussian spatial weight
            float spatialWeight = gaussWeights[abs(x)] * gaussWeights[abs(y)];

            // Depth-aware weight: reject samples across depth discontinuities
            float depthDiff = abs(centerDepth - sampleDepth);
            float depthWeight = 1.0 / (1.0 + depthDiff / depthThreshold);

            float weight = spatialWeight * depthWeight;
            totalAO += sampleAO * weight;
            totalWeight += weight;
        }
    }

    outAO = totalAO / totalWeight;
}
