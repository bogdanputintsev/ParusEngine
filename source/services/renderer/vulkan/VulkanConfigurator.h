#pragma once

#include <cstdint>


namespace parus::vulkan
{

	enum class MSAALevel : uint8_t
	{
		SAMPLES_1 = 1,
		SAMPLES_2 = 2,
		SAMPLES_4 = 4,
		SAMPLES_8 = 8,
		AUTO = 0
	};

	struct VulkanConfigurator
	{
		// Shadow mapping
		uint32_t shadowMapSize = 4096;
		float shadowExtent = 500.0f;
		float shadowNear = 1.0f;
		float shadowFar = 1200.0f;
		float shadowDepthBiasConstant = 1.25f;
		float shadowDepthBiasSlope = 1.75f;

		// SSAO
		bool ssaoEnabled = true;

		// MSAA
		MSAALevel msaaLevel = MSAALevel::AUTO;

		// Camera
		float zNear = 0.1f;
		float zFar = 1500.0f;
		float fieldOfView = 45.0f;

		// Fog
		float fogStart = 500.0f;
		float fogEnd = 2500.0f;

		// Cubemap
		uint32_t cubemapFaceSize = 512;

		// Validation
		bool enableValidationLayers = true;
	};

}
