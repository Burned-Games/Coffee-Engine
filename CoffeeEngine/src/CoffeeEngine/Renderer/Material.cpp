#include "Material.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/MaterialImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Embedded/StandardShader.inl"
#include <cstdint>
#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    Ref<Texture2D> Material::s_MissingTexture;
    Ref<Shader> Material::s_StandardShader;

     Material::Material() : Resource(ResourceType::Material)
    {
        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));

        m_Shader = s_StandardShader;
    }

    Material::Material(const std::string& name)
        : Resource(ResourceType::Material)
    {
        ZoneScoped;

        m_Name = name;

        s_MissingTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));

        m_MaterialTextures.albedo = s_MissingTexture;
        m_MaterialTextureFlags.hasAlbedo = true;

        m_Shader = s_StandardShader;

        m_Shader->Bind();
        m_MaterialTextures.albedo->Bind(0);
        m_Shader->setInt("material.albedoMap", 0);
        m_Shader->Unbind();
    }

    Material::Material(const std::string& name, Ref<Shader> shader) : m_Shader(shader), Resource(ResourceType::Material) {}

    Material::Material(const std::string& name, MaterialTextures& materialTextures)
        : Resource(ResourceType::Material)
    {
        ZoneScoped;

        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));
        
        m_Name = name;

        m_MaterialTextures.albedo = materialTextures.albedo;
        m_MaterialTextures.normal = materialTextures.normal;
        m_MaterialTextures.metallic = materialTextures.metallic;
        m_MaterialTextures.roughness = materialTextures.roughness;
        m_MaterialTextures.ao = materialTextures.ao;
        m_MaterialTextures.emissive = materialTextures.emissive;

        m_MaterialTextureFlags.hasAlbedo = (m_MaterialTextures.albedo != nullptr);
        m_MaterialTextureFlags.hasNormal = (m_MaterialTextures.normal != nullptr);
        m_MaterialTextureFlags.hasMetallic = (m_MaterialTextures.metallic != nullptr);
        m_MaterialTextureFlags.hasRoughness = (m_MaterialTextures.roughness != nullptr);
        m_MaterialTextureFlags.hasAO = (m_MaterialTextures.ao != nullptr);
        m_MaterialTextureFlags.hasEmissive = (m_MaterialTextures.emissive != nullptr);

        if(m_MaterialTextureFlags.hasMetallic)m_MaterialProperties.metallic = 1.0f;
        if(m_MaterialTextureFlags.hasEmissive)m_MaterialProperties.emissive = glm::vec3(1.0f);

        m_Shader = s_StandardShader;

        m_Shader->Bind();
        m_Shader->setInt("material.albedoMap", 0);
        m_Shader->setInt("material.normalMap", 1);
        m_Shader->setInt("material.metallicMap", 2);
        m_Shader->setInt("material.roughnessMap", 3);
        m_Shader->setInt("material.aoMap", 4);
        m_Shader->setInt("material.emissiveMap", 5);
        m_Shader->Unbind();
    }

    Material::Material(ImportData& importData)
        : Resource(ResourceType::Material)
    {
        MaterialImportData& materialImportData = dynamic_cast<MaterialImportData&>(importData);

        if(materialImportData.materialTextures)
        {
            *this = Material(m_Name, *materialImportData.materialTextures);
        }
        else
        {
            *this = Material(m_Name);
        }

        m_Name = materialImportData.name;
        m_UUID = materialImportData.uuid;
        m_FilePath = materialImportData.cachedPath;
    }

    void Material::Use()
    {
        ZoneScoped;

        // Update Texture Flags
        m_MaterialTextureFlags.hasAlbedo = (m_MaterialTextures.albedo != nullptr);
        m_MaterialTextureFlags.hasNormal = (m_MaterialTextures.normal != nullptr);
        m_MaterialTextureFlags.hasMetallic = (m_MaterialTextures.metallic != nullptr);
        m_MaterialTextureFlags.hasRoughness = (m_MaterialTextures.roughness != nullptr);
        m_MaterialTextureFlags.hasAO = (m_MaterialTextures.ao != nullptr);
        m_MaterialTextureFlags.hasEmissive = (m_MaterialTextures.emissive != nullptr);

        m_Shader->Bind();

        // Bind Textures
        if(m_MaterialTextureFlags.hasAlbedo)m_MaterialTextures.albedo->Bind(0);
        if(m_MaterialTextureFlags.hasNormal)m_MaterialTextures.normal->Bind(1);
        if(m_MaterialTextureFlags.hasMetallic)m_MaterialTextures.metallic->Bind(2);
        if(m_MaterialTextureFlags.hasRoughness)m_MaterialTextures.roughness->Bind(3);
        if(m_MaterialTextureFlags.hasAO)m_MaterialTextures.ao->Bind(4);
        if(m_MaterialTextureFlags.hasEmissive)m_MaterialTextures.emissive->Bind(5);

        // Set Material Properties
        m_Shader->setVec4("material.color", m_MaterialProperties.color);
        m_Shader->setFloat("material.metallic", m_MaterialProperties.metallic);
        m_Shader->setFloat("material.roughness", m_MaterialProperties.roughness);
        m_Shader->setFloat("material.ao", m_MaterialProperties.ao);
        m_Shader->setVec3("material.emissive", m_MaterialProperties.emissive);

        // Set Material Texture Flags
        m_Shader->setInt("material.hasAlbedo", m_MaterialTextureFlags.hasAlbedo);
        m_Shader->setInt("material.hasNormal", m_MaterialTextureFlags.hasNormal);
        m_Shader->setInt("material.hasMetallic", m_MaterialTextureFlags.hasMetallic);
        m_Shader->setInt("material.hasRoughness", m_MaterialTextureFlags.hasRoughness);
        m_Shader->setInt("material.hasAO", m_MaterialTextureFlags.hasAO);
        m_Shader->setInt("material.hasEmissive", m_MaterialTextureFlags.hasEmissive);

        m_Shader->setInt("material.transparencyMode", m_MaterialRenderSettings.transparencyMode);
        m_Shader->setFloat("material.alphaCutoff", m_MaterialRenderSettings.alphaCutoff);
    }

    Ref<Material> Material::Create(const std::string& name, MaterialTextures* materialTextures)
    {
        MaterialImportData importData;
        importData.name = name;
        importData.materialTextures = materialTextures;
        importData.uuid = UUID();
        importData.cachedPath = CacheManager::GetCachedFilePath(importData.uuid, ResourceType::Material);

        return ResourceLoader::LoadEmbedded<Material>(importData);
    }
}
