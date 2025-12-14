#include "Storage.h"

#include "engine/EngineCore.h"
#include "services/renderer/vulkan/builder/VulkanTexture2dBuilder.h"

namespace parus
{
    // =============================================
    // Material-related methods
    // =============================================
    
    void Storage::addMaterial(const std::string& materialName, const std::shared_ptr<vulkan::Material>& newMaterial)
    {
        std::scoped_lock lock(materialMutex);
        materials.insert_or_assign(materialName, newMaterial);
    }

    std::shared_ptr<vulkan::Material> Storage::getOrLoadMaterial(
        const std::string& materialName,
        const std::string& diffuseTexturePath,
        const std::string& normalTexturePath,
        const std::string& metallicTexturePath,
        const std::string& roughnessTexturePath,
        const std::string& ambientOcclusionTexturePath)
    {
        if (materialName.empty())
        {
            return nullptr;
        }
        
        if (!hasMaterial(materialName))
        {
            vulkan::Material newMaterial;

            newMaterial.addOrUpdateTexture(vulkan::TextureType::ALBEDO, getOrLoadTexture(diffuseTexturePath));
            newMaterial.addOrUpdateTexture(vulkan::TextureType::NORMAL, getOrLoadTexture(normalTexturePath));
            newMaterial.addOrUpdateTexture(vulkan::TextureType::METALLIC, getOrLoadTexture(metallicTexturePath));
            newMaterial.addOrUpdateTexture(vulkan::TextureType::ROUGHNESS, getOrLoadTexture(roughnessTexturePath));
            newMaterial.addOrUpdateTexture(vulkan::TextureType::AMBIENT_OCCLUSION, getOrLoadTexture(ambientOcclusionTexturePath));
            
            ASSERT(newMaterial.getTexture(vulkan::TextureType::ALBEDO), "Albedo texture must always exist in the model.");
            
            addMaterial(materialName, std::make_shared<vulkan::Material>(newMaterial));

        }

        DEBUG_ASSERT(hasMaterial(materialName), "Material must exist after importing.");
        return getMaterial(materialName);
    }

    bool Storage::hasMaterial(const std::string& materialName) const
    {
        std::scoped_lock lock(materialMutex);
        return materials.contains(materialName);
    }

    std::shared_ptr<vulkan::Material> Storage::getMaterial(const std::string& materialName)
    {
        std::scoped_lock lock(materialMutex);
        return materials[materialName];
    }

    std::shared_ptr<vulkan::Material> Storage::getDefaultMaterial()
    {
        std::scoped_lock lock(materialMutex);
        if (!defaultMaterial)
        {
            LOG_DEBUG("Creating default material");
            defaultMaterial = std::make_shared<vulkan::Material>();
        }
        
        return defaultMaterial;
    }

    std::vector<std::shared_ptr<vulkan::Material>> Storage::getAllMaterials() const
    {
        std::vector<std::shared_ptr<vulkan::Material>> allMaterials;

        {
            std::scoped_lock lock(materialMutex);
            allMaterials.reserve(materials.size());
            for (const auto& [key, material] : materials)
            {
                allMaterials.push_back(material);
            }
        }
        
        return allMaterials;
    }
    
    // =============================================
    // Texture-related methods
    // =============================================
    
    void Storage::addNewTexture(const std::string& path, const std::shared_ptr<vulkan::VulkanTexture2d>& newTexture)
    {
        std::scoped_lock lock(texturesMutex);
        textures.insert_or_assign(path, newTexture);        
    }

    void Storage::setCubemapTexture(const std::shared_ptr<vulkan::VulkanTexture2d>& newCubemapTexture)
    {
        std::scoped_lock lock(texturesMutex);
        ASSERT(!cubemapTexture, "Cubemap texture must be set only once.");
        cubemapTexture = newCubemapTexture;
    }

    std::shared_ptr<vulkan::VulkanTexture2d> Storage::getTexture(const std::string& path)
    {
        std::scoped_lock lock(texturesMutex);
        return textures[path];
    }

    std::shared_ptr<vulkan::VulkanTexture2d> Storage::getDefaultTextureOfType(const vulkan::TextureType textureType)
    {
        if (defaultTextures.size() != vulkan::NUMBER_OF_TEXTURE_TYPES)
        {
            ASSERT(defaultTextures.empty(), "Default textures can be empty or the size of NUMBER_OF_TEXTURE_TYPES.");
            fillDefaultTextures();
        }

        DEBUG_ASSERT(defaultTextures.contains(textureType), "Missing default textures for some types");
        std::scoped_lock lock(texturesMutex);
        return defaultTextures[textureType];
    }

    std::shared_ptr<vulkan::VulkanTexture2d> Storage::getOrLoadTexture(const std::string& texturePath)
    {
        if (texturePath.empty()
            || !std::filesystem::exists(texturePath)
            || !std::filesystem::is_regular_file(texturePath))
        {
            return nullptr;
        }
        
        if (!hasTexture(texturePath))
        {
            vulkan::VulkanTexture2d newTexture 
                = vulkan::VulkanTexture2dBuilder("Texture " + texturePath)
                    .buildFromFile(texturePath);
            addNewTexture(texturePath, std::make_shared<vulkan::VulkanTexture2d>(newTexture));
        }
			
        DEBUG_ASSERT(hasTexture(texturePath), "Texture must exist after importing.");
        return getTexture(texturePath);
    }

    bool Storage::hasTexture(const std::string& path) const
    {
        std::scoped_lock lock(texturesMutex);
        return textures.contains(path);
    }

    std::vector<std::shared_ptr<vulkan::VulkanTexture2d>> Storage::getAllTextures() const
    {
        std::vector<std::shared_ptr<vulkan::VulkanTexture2d>> allTextures;

        {
            std::scoped_lock lock(texturesMutex);
            allTextures.reserve(textures.size() + defaultTextures.size());
            for (const auto& [key, texture] : textures)
            {
                allTextures.push_back(texture);
            }

            for (const auto& [textureType, texture] : defaultTextures)
            {
                allTextures.push_back(texture);
            }
            
            if (cubemapTexture)
            {
                allTextures.push_back(cubemapTexture);
            }
        }
        
        return allTextures;
    }

    // =============================================
    // Mesh-related methods
    // =============================================

    void Storage::addNewMesh(const std::string& path, const std::shared_ptr<Mesh>& newMesh)
    {
        std::scoped_lock lock(meshesMutex);
        meshes.insert_or_assign(path, newMesh);   
    }

    std::shared_ptr<Mesh> Storage::getMeshByPath(const std::string& path)
    {
        std::scoped_lock lock(meshesMutex);
        return meshes[path];
    }

    std::vector<std::shared_ptr<Mesh>> Storage::getAllMeshes() const
    {
        std::vector<std::shared_ptr<Mesh>> allMeshes;

        {
            std::scoped_lock lock(meshesMutex);
            allMeshes.reserve(meshes.size());
            for (const auto& [key, mesh] : meshes)
            {
                allMeshes.push_back(mesh);
            }
        }
        
        return allMeshes;
    }

    std::vector<std::shared_ptr<Mesh>> Storage::getAllMeshesByType(const MeshType meshType) const
    {
        std::vector<std::shared_ptr<Mesh>> allMeshes;

        {
            std::scoped_lock lock(meshesMutex);
            allMeshes.reserve(meshes.size());
            for (const auto& [key, mesh] : meshes)
            {
                if (mesh->meshType == meshType)
                {
                    allMeshes.push_back(mesh);
                }
            }
        }
        
        return allMeshes;
    }

    void Storage::fillDefaultTextures()
    {
        ASSERT(defaultTextures.empty(), "Default textures must be empty in the beginning.");

        vulkan::Material::iterateAllTextureTypes([&](const vulkan::TextureType textureType)
        {
            vulkan::VulkanTexture2d defaultTexture;
                
            switch (textureType)
            {
            case vulkan::TextureType::ALBEDO:
                defaultTexture = vulkan::VulkanTexture2dBuilder("Default Albedo Map")
                    .buildFromSolidColor(math::Vector3(1.0f, 1.0f, 1.0f));
                break;
            case vulkan::TextureType::AMBIENT_OCCLUSION:
                defaultTexture = vulkan::VulkanTexture2dBuilder("Default AO Map")
                    .buildFromSolidColor(math::Vector3(1.0f, 1.0f, 1.0f));
                break;
            case vulkan::TextureType::NORMAL:
                defaultTexture = vulkan::VulkanTexture2dBuilder("Default Normal Map")
                    .buildFromSolidColor(math::Vector3(0.5f, 0.5f, 1.0f));
                break;
            case vulkan::TextureType::METALLIC:
                defaultTexture = vulkan::VulkanTexture2dBuilder("Default Metallic Map")
                    .buildFromSolidColor(math::Vector3(0.0f, 0.0f, 0.0f));
                break;
            case vulkan::TextureType::ROUGHNESS:
                defaultTexture = vulkan::VulkanTexture2dBuilder("Default Roughness Map")
                    .buildFromSolidColor(math::Vector3(0.0f, 0.0f, 0.0f));
                break;
            }
                
            defaultTextures[textureType] = std::make_shared<vulkan::VulkanTexture2d>(defaultTexture);
        });

        ASSERT(defaultTextures.size() == vulkan::NUMBER_OF_TEXTURE_TYPES,
            "Missing default textures for some texture types.");
    }
}
