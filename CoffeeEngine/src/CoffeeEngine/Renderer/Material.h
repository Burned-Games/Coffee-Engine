#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/Serialization/GLMSerialization.h"
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
             archive(transparencyMode, alphaCutoff, blendMode, cullMode, depthTest, wireframe);
         }
     };

    class Material : public Resource
    {
    public:
        Material() : Resource(ResourceType::Material) {}

        Material(ResourceType type) : Resource(type) {}

        Material(const std::string& name, ResourceType type) : Resource(type) { m_Name = name; }

        Material(const std::string& name, Ref<Shader> shader) : Resource(ResourceType::Material), m_Shader(shader) { m_Name = name; }

        Ref<Shader> GetShader() { return m_Shader; }
        MaterialRenderSettings& GetRenderSettings() { return m_RenderSettings; }

        virtual void Use() = 0;
    private:
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::base_class<Resource>(this), m_RenderSettings);
        }
        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::base_class<Resource>(this), m_RenderSettings);
        }
    protected:
        Ref<Shader> m_Shader; ///< The shader used by the material.
        MaterialRenderSettings m_RenderSettings; ///< The render settings for the material.
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
                archive(color, metallic, roughness, ao, emissive);
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
                UUID albedoUUID = albedo ? albedo->GetUUID() : UUID::null;
                UUID normalUUID = normal ? normal->GetUUID() : UUID::null;
                UUID metallicUUID = metallic ? metallic->GetUUID() : UUID::null;
                UUID roughnessUUID = roughness ? roughness->GetUUID() : UUID::null;
                UUID aoUUID = ao ? ao->GetUUID() : UUID::null;
                UUID emissiveUUID = emissive ? emissive->GetUUID() : UUID::null;

                archive(albedoUUID, normalUUID, metallicUUID, roughnessUUID, aoUUID, emissiveUUID);
            }

            template<class Archive>
            void load(Archive& archive)
            {
                UUID albedoUUID, normalUUID, metallicUUID, roughnessUUID, aoUUID, emissiveUUID;
                archive(albedoUUID, normalUUID, metallicUUID, roughnessUUID, aoUUID, emissiveUUID);

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
                archive(hasAlbedo, hasNormal, hasMetallic, hasRoughness, hasAO, hasEmissive);
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
            archive(m_Textures, m_TextureFlags, m_Properties, m_RenderSettings, cereal::base_class<Material>(this));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(m_Textures, m_TextureFlags, m_Properties, m_RenderSettings, cereal::base_class<Material>(this));
        }

        template<class Archive>
        static void load_and_construct(Archive& archive, cereal::construct<PBRMaterial>& construct)
        {
            PBRMaterialTextures PBRMaterialTextures;
            PBRMaterialTextureFlags PBRMaterialTextureFlags;
            PBRMaterialProperties PBRMaterialProperties;

            archive(PBRMaterialTextures, PBRMaterialTextureFlags, PBRMaterialProperties, cereal::base_class<Material>(construct.ptr()));

            // TODO: TEST THE GETNAME PART PLEASE!!
            construct(construct.ptr()->GetName(), PBRMaterialTextures);
            construct->m_TextureFlags = PBRMaterialTextureFlags;
            construct->m_Properties = PBRMaterialProperties;
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