#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

#include "renderer/vulkan/mesh/Mesh.h"

namespace tessera
{
    
    class Storage final
    {
    public:
        void addNewTexture(const std::string& path, const std::shared_ptr<Texture>& newTexture);
        std::shared_ptr<Texture> getTexture(const std::string& path);
        bool containsTexture(const std::string& path) const;
        std::vector<std::shared_ptr<Texture>> getTextures() const;

        void addNewMesh(const std::string& path, const std::shared_ptr<Mesh>& newMesh);
        std::shared_ptr<Mesh> getMeshByPath(const std::string& path);
        std::vector<std::shared_ptr<Mesh>> getMeshes() const;
        
    private:
        std::unordered_map<std::string, std::shared_ptr<Texture>> textures;
        mutable std::mutex texturesMutex;
        
        std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
        mutable std::mutex meshesMutex;
    };

    
}
