#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

namespace tessera
{
	struct Vector2
	{
		float x, y;

		bool operator==(const Vector2& other) const
		{
			return  x == other.x && y == other.y;
		}
	};

	struct Vector3
	{
		float x, y, z;

		bool operator==(const Vector3& other) const
		{
			return x == other.x && y == other.y && z == other.z;
		}
	};

	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 color;
		glm::vec2 texCoord;

		bool operator==(const Vertex& other) const
		{
			return position == other.position && color == other.color && texCoord == other.texCoord;
		}


	};

}


namespace std {
	template<> struct hash<tessera::Vertex> {
		size_t operator()(tessera::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}