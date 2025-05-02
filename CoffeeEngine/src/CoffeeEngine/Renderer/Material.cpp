#include "Material.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/ImportData/MaterialImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Embedded/StandardShader.inl"
#include <glm/fwd.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    ShaderMaterial::ShaderMaterial(ImportData& importData)
        : Material(ResourceType::ShaderMaterial)
    {
        ZoneScoped;

/*         ShaderMaterialImportData& shaderMaterialImportData = dynamic_cast<ShaderMaterialImportData&>(importData);

        m_Name = shaderMaterialImportData.name;
        m_UUID = shaderMaterialImportData.uuid;
        m_FilePath = shaderMaterialImportData.cachedPath;

        m_Shader = Shader::Create(shaderMaterialImportData.shaderPath); */

        m_UUID = importData.uuid;
        m_Name = importData.originalPath.stem().string();
        m_Shader = Shader::Create(importData.originalPath);
        m_FilePath = importData.cachedPath;
    }

    void ShaderMaterial::Use()
    {
        ZoneScoped;

        if (m_Shader)
        {
            m_Shader->Bind();
        }
    }

    Ref<ShaderMaterial> ShaderMaterial::Create(const std::string& name, Ref<Shader> shader)
    {
        return CreateRef<ShaderMaterial>(name, shader);
    }

    Ref<Texture2D> PBRMaterial::s_MissingTexture;
    Ref<Shader> PBRMaterial::s_StandardShader;

     PBRMaterial::PBRMaterial() : Material(ResourceType::PBRMaterial)
    {
        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));

        m_Shader = s_StandardShader;
    }

    PBRMaterial::PBRMaterial(const std::string& name)
        : Material(name, ResourceType::PBRMaterial)
    {
        ZoneScoped;

        s_MissingTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));

        m_Textures.albedo = s_MissingTexture;
        m_TextureFlags.hasAlbedo = true;

        m_Shader = s_StandardShader;

        m_Shader->Bind();
        m_Textures.albedo->Bind(0);
        m_Shader->setInt("material.albedoMap", 0);
        m_Shader->Unbind();
    }

    PBRMaterial::PBRMaterial(const std::string& name, PBRMaterialTextures& materialTextures)
        : Material(ResourceType::PBRMaterial)
    {
        ZoneScoped;

        s_StandardShader  = s_StandardShader ? s_StandardShader : CreateRef<Shader>("StandardShader", std::string(standardShaderSource));
        
        m_Name = name;

        m_Textures.albedo = materialTextures.albedo;
        m_Textures.normal = materialTextures.normal;
        m_Textures.metallic = materialTextures.metallic;
        m_Textures.roughness = materialTextures.roughness;
        m_Textures.ao = materialTextures.ao;
        m_Textures.emissive = materialTextures.emissive;

        m_TextureFlags.hasAlbedo = (m_Textures.albedo != nullptr);
        m_TextureFlags.hasNormal = (m_Textures.normal != nullptr);
        m_TextureFlags.hasMetallic = (m_Textures.metallic != nullptr);
        m_TextureFlags.hasRoughness = (m_Textures.roughness != nullptr);
        m_TextureFlags.hasAO = (m_Textures.ao != nullptr);
        m_TextureFlags.hasEmissive = (m_Textures.emissive != nullptr);

        if(m_TextureFlags.hasMetallic)m_Properties.metallic = 1.0f;
        if(m_TextureFlags.hasEmissive)m_Properties.emissive = glm::vec3(1.0f);

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

    PBRMaterial::PBRMaterial(ImportData& importData)
        : Material(ResourceType::PBRMaterial)
    {
        PBRMaterialImportData& materialImportData = dynamic_cast<PBRMaterialImportData&>(importData);

        if(materialImportData.materialTextures)
        {
            *this = PBRMaterial(m_Name, *materialImportData.materialTextures);
        }
        else
        {
            *this = PBRMaterial(m_Name);
        }

        m_Name = materialImportData.name;
        m_UUID = materialImportData.uuid;
        m_FilePath = materialImportData.cachedPath;
    }

    void PBRMaterial::Use()
    {
        ZoneScoped;

        // Update Texture Flags
        m_TextureFlags.hasAlbedo = (m_Textures.albedo != nullptr);
        m_TextureFlags.hasNormal = (m_Textures.normal != nullptr);
        m_TextureFlags.hasMetallic = (m_Textures.metallic != nullptr);
        m_TextureFlags.hasRoughness = (m_Textures.roughness != nullptr);
        m_TextureFlags.hasAO = (m_Textures.ao != nullptr);
        m_TextureFlags.hasEmissive = (m_Textures.emissive != nullptr);

        m_Shader->Bind();

        // Bind Textures
        if(m_TextureFlags.hasAlbedo)m_Textures.albedo->Bind(0);
        if(m_TextureFlags.hasNormal)m_Textures.normal->Bind(1);
        if(m_TextureFlags.hasMetallic)m_Textures.metallic->Bind(2);
        if(m_TextureFlags.hasRoughness)m_Textures.roughness->Bind(3);
        if(m_TextureFlags.hasAO)m_Textures.ao->Bind(4);
        if(m_TextureFlags.hasEmissive)m_Textures.emissive->Bind(5);

        // Set Material Properties
        m_Shader->setVec4("material.color", m_Properties.color);
        m_Shader->setFloat("material.metallic", m_Properties.metallic);
        m_Shader->setFloat("material.roughness", m_Properties.roughness);
        m_Shader->setFloat("material.ao", m_Properties.ao);
        m_Shader->setVec3("material.emissive", m_Properties.emissive);

        // Set Material Texture Flags
        m_Shader->setInt("material.hasAlbedo", m_TextureFlags.hasAlbedo);
        m_Shader->setInt("material.hasNormal", m_TextureFlags.hasNormal);
        m_Shader->setInt("material.hasMetallic", m_TextureFlags.hasMetallic);
        m_Shader->setInt("material.hasRoughness", m_TextureFlags.hasRoughness);
        m_Shader->setInt("material.hasAO", m_TextureFlags.hasAO);
        m_Shader->setInt("material.hasEmissive", m_TextureFlags.hasEmissive);

        m_Shader->setInt("material.transparencyMode", m_RenderSettings.transparencyMode);
        m_Shader->setFloat("material.alphaCutoff", m_RenderSettings.alphaCutoff);
    }

    Ref<PBRMaterial> PBRMaterial::Create(const std::string& name, PBRMaterialTextures* materialTextures)
    {
        PBRMaterialImportData importData;
        importData.name = name;
        importData.materialTextures = materialTextures;
        importData.uuid = UUID();
        importData.cachedPath = CacheManager::GetCachedFilePath(importData.uuid, ResourceType::Material);

        return ResourceLoader::LoadEmbedded<PBRMaterial>(importData);
    }
}
