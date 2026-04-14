#pragma once
#include <mutex>
#include <string>
#include <unordered_map>

#include "services/renderer/Material.h"
#include "services/renderer/Texture.h"
#include "services/renderer/TextureType.h"
#include "services/renderer/vulkan/mesh/Mesh.h"

namespace parus
{

    class Storage final
    {
    public:
        // --- Materials ---
        void addMaterial(const std::string& materialName, const std::shared_ptr<parus::Material>& newMaterial);
        std::shared_ptr<parus::Material> getOrLoadMaterial(
            const std::string& materialName,
            const std::string& diffuseTexturePath,
            const std::string& normalTexturePath = "",
            const std::string& metallicTexturePath = "",
            const std::string& roughnessTexturePath = "",
            const std::string& ambientOcclusionTexturePath = "");
        bool hasMaterial(const std::string& materialName) const;
        std::shared_ptr<parus::Material> getMaterial(const std::string& materialName);
        std::shared_ptr<parus::Material> getDefaultMaterial();
        std::vector<std::shared_ptr<parus::Material>> getAllMaterials() const;

        // --- Textures ---
        void addNewTexture(const std::string& path, const std::shared_ptr<parus::Texture>& newTexture);
        void setCubemapTexture(const std::shared_ptr<parus::Texture>& newCubemapTexture);
        std::shared_ptr<parus::Texture> getTexture(const std::string& path);
        std::shared_ptr<parus::Texture> getDefaultTextureOfType(const parus::TextureType textureType);
        std::shared_ptr<parus::Texture> getOrLoadTexture(const std::string& texturePath, parus::TextureType textureType = parus::TextureType::ALBEDO);
        bool hasTexture(const std::string& path) const;
        std::vector<std::shared_ptr<parus::Texture>> getAllTextures() const;

        // --- Meshes ---
        void addNewMesh(const std::string& path, const std::shared_ptr<Mesh>& newMesh);
        std::shared_ptr<Mesh> getMeshByPath(const std::string& path);
        std::vector<std::shared_ptr<Mesh>> getAllMeshes() const;
        std::vector<std::shared_ptr<Mesh>> getAllMeshesByType(const MeshType meshType) const;

    private:
        std::unordered_map<std::string, std::shared_ptr<parus::Material>> materials;
        std::shared_ptr<parus::Material> defaultMaterial;

        mutable std::mutex materialMutex;


        std::unordered_map<std::string, std::shared_ptr<parus::Texture>> textures;
        std::unordered_map<parus::TextureType, std::shared_ptr<parus::Texture>> defaultTextures;
        std::shared_ptr<parus::Texture> cubemapTexture;

        void fillDefaultTextures();
        mutable std::mutex texturesMutex;
        
        
        std::unordered_map<std::string, std::shared_ptr<Mesh>> meshes;
        mutable std::mutex meshesMutex;
    };

    
}
