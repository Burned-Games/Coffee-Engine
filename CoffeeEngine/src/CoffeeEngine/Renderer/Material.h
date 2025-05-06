#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/IO/Serialization/GLMSerialization.h"
#include "CoffeeEngine/IO/Serialization/FilesystemPathSerialization.h"
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <filesystem>
#include <glm/fwd.hpp>
#include <string>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

     struct MaterialRenderSettings
     {
         // Transparency
         enum TransparencyMode
         {
             Disabled = 0,
             Alpha,
             AlphaCutoff
         } transparencyMode = TransparencyMode::Disabled; ///< The transparency mode.
 
         float alphaCutoff = 0.5f; ///< The alpha cutoff value for the transparency mode.
 
         // Blend mode
         enum BlendMode
         {
             Mix = 0,
             Add,
             Subtract,
             Multiply
         } blendMode = BlendMode::Mix; ///< The blend mode for the transparency.
 
         // Culling
         enum CullMode
         {
             Front = 0,
             Back,
             None
         } cullMode = CullMode::Back; ///< The culling mode for the PBRMaterial.
 
         // Depth
 /*         enum DepthMode
         {
             Read = 0,
             Write,
             None
         } depthMode = DepthMode::Write; ///< The depth mode for the PBRMaterial. */
         bool depthTest = true; ///< Whether to enable depth testing.
 
         // Wireframe
         bool wireframe = false; ///< Whether to render the PBRMaterial in wireframe mode.
         
     private:
         friend class cereal::access;
 
         template<class Archive>
         void serialize(Archive& archive)
         {
            archive(
                cereal::make_nvp("TransparencyMode", transparencyMode),
                cereal::make_nvp("AlphaCutoff", alphaCutoff),
                cereal::make_nvp("BlendMode", blendMode),
                cereal::make_nvp("CullMode", cullMode),
                cereal::make_nvp("DepthTest", depthTest),
                cereal::make_nvp("Wireframe", wireframe)
            );
         }
     };

    class Material : public Resource
    {
    public:
        Material() : Resource(ResourceType::Material) {}

        Material(ResourceType type) : Resource(type) {}

        Material(const std::string& name, ResourceType type) : Resource(type) { m_Name = name; }

        Material(const std::string& name, Ref<Shader> shader, ResourceType type) : Resource(type), m_Shader(shader) { m_Name = name; }

        Ref<Shader> GetShader() { return m_Shader; }
        MaterialRenderSettings& GetRenderSettings() { return m_RenderSettings; }

        virtual void Use() = 0;
    private:
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            std::filesystem::path shaderPath = m_Shader ? m_Shader->GetPath() : "";
            
            if (Project::GetActive())
                shaderPath = std::filesystem::relative(m_Shader->GetPath(), Project::GetActive()->GetProjectDirectory());
            else
                shaderPath = m_Shader->GetPath();

            archive(cereal::base_class<Resource>(this), cereal::make_nvp("Shader Path", shaderPath.generic_string()), cereal::make_nvp("Render Settings", m_RenderSettings));
        }
        template<class Archive>
        void load(Archive& archive)
        {
            std::string shaderPath;
            archive(cereal::base_class<Resource>(this), cereal::make_nvp("Shader Path", shaderPath), cereal::make_nvp("Render Settings", m_RenderSettings));

            if (Project::GetActive())
                shaderPath = (Project::GetActive()->GetProjectDirectory() / shaderPath).string();
            else
                shaderPath = shaderPath;

            if (!shaderPath.empty() and std::filesystem::is_regular_file(shaderPath)) m_Shader = Shader::Create(shaderPath);
        }
    protected:
        Ref<Shader> m_Shader; ///< The shader used by the material.
        MaterialRenderSettings m_RenderSettings; ///< The render settings for the material.
    };

    class ShaderMaterial : public Material
    {
    public:
        ShaderMaterial() : Material(ResourceType::ShaderMaterial) {}

        ShaderMaterial(const std::string& name, Ref<Shader> shader) : Material(name, shader, ResourceType::ShaderMaterial) {}

        ShaderMaterial(ImportData& importData);

        void Use() override;

        void SetShader(Ref<Shader> shader) { m_Shader = shader; }

        static Ref<ShaderMaterial> Create(const std::string& name = "", Ref<Shader> shader = nullptr);
    private:
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::base_class<Material>(this));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::base_class<Material>(this));
        }
    private:
    };

    /**
     * @brief Structure representing the properties of a PBRMaterial.
     */
    struct PBRMaterialProperties
    {
        glm::vec4 color = glm::vec4(1.0f); ///< The color of the PBRMaterial.
        float metallic = 0.0f; ///< The metallic value of the PBRMaterial.
        float roughness = 1.0f; ///< The roughness value of the PBRMaterial.
        float ao = 1.0f; ///< The ambient occlusion value of the PBRMaterial.
        glm::vec3 emissive = glm::vec3(0.0f); ///< The emissive value of the PBRMaterial.

        private:
            friend class cereal::access;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(
                    cereal::make_nvp("Color", color),
                    cereal::make_nvp("Metallic", metallic),
                    cereal::make_nvp("Roughness", roughness),
                    cereal::make_nvp("AO", ao),
                    cereal::make_nvp("Emissive", emissive)
                );
            }
    };

    /**
     * @brief Structure representing the textures used in a PBRMaterial.
     */
    struct PBRMaterialTextures // Temporal
    {
        Ref<Texture2D> albedo; ///< The albedo texture.
        Ref<Texture2D> normal; ///< The normal map texture.
        Ref<Texture2D> metallic; ///< The metallic texture.
        Ref<Texture2D> roughness; ///< The roughness texture.
        Ref<Texture2D> ao; ///< The ambient occlusion texture.
        Ref<Texture2D> emissive; ///< The emissive texture.

        private:
            friend class cereal::access;

            template<class Archive>
            void save(Archive& archive) const
            {
                archive(
                    cereal::make_nvp("AlbedoUUID", albedo ? albedo->GetUUID() : UUID::null),
                    cereal::make_nvp("NormalUUID", normal ? normal->GetUUID() : UUID::null),
                    cereal::make_nvp("MetallicUUID", metallic ? metallic->GetUUID() : UUID::null),
                    cereal::make_nvp("RoughnessUUID", roughness ? roughness->GetUUID() : UUID::null),
                    cereal::make_nvp("AOUUID", ao ? ao->GetUUID() : UUID::null),
                    cereal::make_nvp("EmissiveUUID", emissive ? emissive->GetUUID() : UUID::null)
                );
            }

            template<class Archive>
            void load(Archive& archive)
            {
                UUID albedoUUID, normalUUID, metallicUUID, roughnessUUID, aoUUID, emissiveUUID;
                archive(
                    cereal::make_nvp("AlbedoUUID", albedoUUID),
                    cereal::make_nvp("NormalUUID", normalUUID),
                    cereal::make_nvp("MetallicUUID", metallicUUID),
                    cereal::make_nvp("RoughnessUUID", roughnessUUID),
                    cereal::make_nvp("AOUUID", aoUUID),
                    cereal::make_nvp("EmissiveUUID", emissiveUUID)
                );

                albedo = ResourceLoader::GetResource<Texture2D>(albedoUUID);
                normal = ResourceLoader::GetResource<Texture2D>(normalUUID);
                metallic = ResourceLoader::GetResource<Texture2D>(metallicUUID);
                roughness = ResourceLoader::GetResource<Texture2D>(roughnessUUID);
                ao = ResourceLoader::GetResource<Texture2D>(aoUUID);
                emissive = ResourceLoader::GetResource<Texture2D>(emissiveUUID);
            }
    };

    struct PBRMaterialTextureFlags
    {
        bool hasAlbedo = false; ///< Whether the PBRMaterial has an albedo texture.
        bool hasNormal = false; ///< Whether the PBRMaterial has a normal map texture.
        bool hasMetallic = false; ///< Whether the PBRMaterial has a metallic texture.
        bool hasRoughness = false; ///< Whether the PBRMaterial has a roughness texture.
        bool hasAO = false; ///< Whether the PBRMaterial has an ambient occlusion texture.
        bool hasEmissive = false; ///< Whether the PBRMaterial has an emissive texture.

        private:
            friend class cereal::access;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(
                    cereal::make_nvp("HasAlbedo", hasAlbedo),
                    cereal::make_nvp("HasNormal", hasNormal),
                    cereal::make_nvp("HasMetallic", hasMetallic),
                    cereal::make_nvp("HasRoughness", hasRoughness),
                    cereal::make_nvp("HasAO", hasAO),
                    cereal::make_nvp("HasEmissive", hasEmissive)
                );
            }
    };

    /**
     * @brief Class representing a PBRMaterial.
     */
    class PBRMaterial : public Material
    {
    public:

        PBRMaterial();
        /**
         * @brief Default destructor for the PBRMaterial class.
         */
         ~PBRMaterial() = default;

        /**
         * @brief Default constructor for the PBRPBRMaterial class.
         */
        PBRMaterial(const std::string& name);

        /**
         * @brief Constructs a PBRMaterial with the specified shader.
         * @param shader The shader to be used with the PBRMaterial.
         */
        PBRMaterial(const std::string& name, Ref<Shader> shader);

        /**
         * @brief Constructs a PBRMaterial with the specified textures.
         * @param PBRMaterialTextures The textures to be used with the PBRMaterial.
         */
        PBRMaterial(const std::string& name, PBRMaterialTextures& PBRMaterialTextures);

        PBRMaterial(ImportData& importData);

        void Use() override;

        PBRMaterialTextures& GetTextures() { return m_Textures; }
        PBRMaterialProperties& GetProperties() { return m_Properties; }

        //TODO: Remove the PBRMaterialTextures parameter and make a function that set the PBRMaterialTextures and the shader too
        static Ref<PBRMaterial> Create(const std::string& name = "", PBRMaterialTextures* PBRMaterialTextures = nullptr);

    private:
        
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(
                cereal::make_nvp("Textures", m_Textures),
                cereal::make_nvp("TextureFlags", m_TextureFlags),
                cereal::make_nvp("Properties", m_Properties),
                cereal::base_class<Material>(this)
            );
        }
        
        template<class Archive>
        void load(Archive& archive)
        {
            archive(
                cereal::make_nvp("Textures", m_Textures),
                cereal::make_nvp("TextureFlags", m_TextureFlags),
                cereal::make_nvp("Properties", m_Properties),
                cereal::base_class<Material>(this)
            );
        }

        template<class Archive>
        static void load_and_construct(Archive& archive, cereal::construct<PBRMaterial>& construct)
        {
            PBRMaterialTextures PBRMaterialTextures;
            PBRMaterialTextureFlags PBRMaterialTextureFlags;
            PBRMaterialProperties PBRMaterialProperties;
            PBRMaterial tmpMaterial;

            archive(PBRMaterialTextures, PBRMaterialTextureFlags, PBRMaterialProperties, cereal::base_class<Material>(&tmpMaterial));

            construct(tmpMaterial.GetName(), PBRMaterialTextures);
            construct->m_TextureFlags = PBRMaterialTextureFlags;
            construct->m_Properties = PBRMaterialProperties;
            construct->m_Name = tmpMaterial.GetName();
            construct->m_FilePath = tmpMaterial.GetPath();
            construct->m_Type = tmpMaterial.GetType();
            construct->m_UUID = tmpMaterial.GetUUID();
        }

    private:
        PBRMaterialTextures m_Textures; ///< The textures used in the PBRMaterial.
        PBRMaterialTextureFlags m_TextureFlags; ///< The flags for the textures used in the PBRMaterial.
        PBRMaterialProperties m_Properties; ///< The properties of the PBRMaterial.
        static Ref<Texture2D> s_MissingTexture; ///< The texture to use when a texture is missing.
        static Ref<Shader> s_StandardShader; ///< The standard shader to use with the PBRMaterial. (When the Material be a base class of PBRMaterial and ShaderMaterial this should be moved to PBRMaterial)
    };

    /** @} */
}

CEREAL_REGISTER_TYPE(Coffee::Material);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Material);
CEREAL_REGISTER_TYPE(Coffee::PBRMaterial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Material, Coffee::PBRMaterial);
CEREAL_REGISTER_TYPE(Coffee::ShaderMaterial);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Material, Coffee::ShaderMaterial);