#version 450

layout(set = 0, binding = 0) uniform sampler2D ssaoInput;

layout(location = 0) in vec2 inUV;
layout(location = 0) out float outAO;

void main()
{
    vec2 texelSize = 1.0 / textureSize(ssaoInput, 0);
    float result = 0.0;

    for (int x = -2; x <= 2; x++)
    {
        for (int y = -2; y <= 2; y++)
        {
            result += texture(ssaoInput, inUV + vec2(float(x), float(y)) * texelSize).r;
        }
    }

    outAO = result / 25.0;
}
