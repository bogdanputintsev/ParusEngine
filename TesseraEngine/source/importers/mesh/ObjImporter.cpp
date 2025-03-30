#include "ObjImporter.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include "Core.h"

namespace tessera
{
    
    Mesh ObjImporter::importFromFile(const std::string& filePath)
    {
	    ASSERT(std::filesystem::exists(filePath),
			"File " + filePath + " must exist.");
    	ASSERT(std::filesystem::is_regular_file(filePath),
			"File " + filePath + " must be a regular file.");
	    ASSERT(utils::string::equalsIgnoreCase(std::filesystem::path(filePath).extension().string(), ".OBJ"),
			"File " + filePath + " must have OBJ extension");
    	
    	Mesh newMesh{};

		LOG_INFO("Loading mesh: " + filePath);

		// Load the model (vertices and indices)
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		size_t lastSlash = filePath.find_last_of("/\\");
		std::string baseDir = filePath.substr(0, lastSlash + 1);

		ASSERT(tinyobj::LoadObj(
			&attrib, &shapes, &materials, &warn, &err,
			filePath.c_str(),
			baseDir.c_str(),
			true),
			warn + err);

		std::vector<std::shared_ptr<Texture>> modelTextures;
		
		for (const auto& material : materials)
		{
			const std::string textureName = material.diffuse_texname;
			if (!textureName.empty())
			{
				const std::string texturePath = baseDir + textureName;

				std::shared_ptr<Texture> currentTexture;
				if (!CORE->world.getStorage()->containsTexture(texturePath))
				{
					Texture newTexture = TextureImporter::importFromFile(texturePath);
					CORE->world.getStorage()->addNewTexture(texturePath, std::make_shared<Texture>(newTexture));
				}
				
				DEBUG_ASSERT(CORE->world.getStorage()->containsTexture(texturePath), "Texture must exist after importing.");
				currentTexture = CORE->world.getStorage()->getTexture(texturePath);

				modelTextures.push_back(currentTexture);
			}
		}

		std::unordered_map<int, MeshPart> materialMeshes;
		std::unordered_map<math::Vertex, uint32_t> uniqueVertices;
		
		for (const auto& shape : shapes) 
		{
			size_t indexOffset = 0;

			// Process each face in the shape.
			for (size_t faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); faceIndex++)
			{
				int materialId = shape.mesh.material_ids[faceIndex];

				if (!materialMeshes.contains(materialId))
				{
					MeshPart newMeshPart;
					newMeshPart.texture = modelTextures[materialId];
					materialMeshes[materialId] = newMeshPart;
				}

				MeshPart& currentMesh = materialMeshes[materialId];
				
				size_t faceVertices = 3;
				for (size_t v = 0; v < faceVertices; v++)
				{
					tinyobj::index_t index = shape.mesh.indices[indexOffset + v];

					math::Vertex vertex{};
			
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

					if (index.texcoord_index >= 0)
					{
						vertex.textureCoordinates = math::Vector2(
							attrib.texcoords[2 * index.texcoord_index + 0],
							1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
						);
					}
					else
					{
						vertex.textureCoordinates = {0.0f, 0.0f};
					}
					
					vertex.color = { 1.0f, 1.0f, 1.0f };

					if (!uniqueVertices.contains(vertex))
					{
						uniqueVertices[vertex] = static_cast<uint32_t>(currentMesh.vertices.size());
						currentMesh.vertices.push_back(vertex);
					}
			
					currentMesh.indices.push_back(uniqueVertices[vertex]);
				}
				indexOffset += faceVertices;
			}

			uniqueVertices.clear();
		}

		// Convert temporary meshes to final model meshes
		for (auto& [matId, mesh] : materialMeshes)
		{
			mesh.vertexCount = mesh.vertices.size();
			mesh.indexCount = mesh.indices.size();
			newMesh.meshParts.push_back(mesh);
		}

    	return newMesh;
    }
}
