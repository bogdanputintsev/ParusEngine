#include "Storage.h"

namespace tessera
{
    void Storage::addNewTexture(const std::string& path, const std::shared_ptr<Texture>& newTexture)
    {
        std::lock_guard lock(texturesMutex);
        textures.insert_or_assign(path, newTexture);        
    }

    std::shared_ptr<Texture> Storage::getTexture(const std::string& path)
    {
        std::lock_guard lock(texturesMutex);
        return textures[path];
    }

    bool Storage::containsTexture(const std::string& path) const
    {
        std::lock_guard lock(texturesMutex);
        return textures.contains(path);
    }

    std::vector<std::shared_ptr<Texture>> Storage::getTextures() const
    {
        std::vector<std::shared_ptr<Texture>> allTextures;

        {
            std::lock_guard lock(texturesMutex);
            allTextures.reserve(textures.size());
            for (const auto& [key, texture] : textures)
            {
                allTextures.push_back(texture);
            }
        }
        
        return allTextures;
    }

    void Storage::addNewMesh(const std::string& path, const std::shared_ptr<Mesh>& newMesh)
    {
        std::lock_guard lock(meshesMutex);
        meshes.insert_or_assign(path, newMesh);   
    }

    std::shared_ptr<Mesh> Storage::getMeshByPath(const std::string& path)
    {
        std::lock_guard lock(meshesMutex);
        return meshes[path];
    }

    std::vector<std::shared_ptr<Mesh>> Storage::getMeshes() const
    {
        std::vector<std::shared_ptr<Mesh>> allMeshes;

        {
            std::lock_guard lock(meshesMutex);
            allMeshes.reserve(meshes.size());
            for (const auto& [key, mesh] : meshes)
            {
                allMeshes.push_back(mesh);
            }
        }
        
        return allMeshes;
    }
}
