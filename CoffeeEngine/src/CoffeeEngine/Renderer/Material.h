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

    // Not implemented yet!!
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
        } cullMode = CullMode::Back; ///< The culling mode for the material.

        // Depth
/*         enum DepthMode
        {
            Read = 0,
            Write,
            None
        } depthMode = DepthMode::Write; ///< The depth mode for the material. */
        bool depthTest = true; ///< Whether to enable depth testing.

        // Wireframe
        bool wireframe = false; ///< Whether to render the material in wireframe mode.
        
    private:
        friend class cereal::access;

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(transparencyMode, alphaCutoff, blendMode, cullMode, depthTest, wireframe);
        }
    };

    /**
     * @brief Structure representing the properties of a material.
     */
    struct MaterialProperties
    {
        glm::vec4 color = glm::vec4(1.0f); ///< The color of the material.
        float metallic = 0.0f; ///< The metallic value of the material.
        float roughness = 1.0f; ///< The roughness value of the material.
        float ao = 1.0f; ///< The ambient occlusion value of the material.
        glm::vec3 emissive = glm::vec3(0.0f); ///< The emissive value of the material.

        private:
            friend class cereal::access;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(color, metallic, roughness, ao, emissive);
            }
    };

    /**
     * @brief Structure representing the textures used in a material.
     */
    struct MaterialTextures // Temporal
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

    struct MaterialTextureFlags
    {
        bool hasAlbedo = false; ///< Whether the material has an albedo texture.
        bool hasNormal = false; ///< Whether the material has a normal map texture.
        bool hasMetallic = false; ///< Whether the material has a metallic texture.
        bool hasRoughness = false; ///< Whether the material has a roughness texture.
        bool hasAO = false; ///< Whether the material has an ambient occlusion texture.
        bool hasEmissive = false; ///< Whether the material has an emissive texture.

        private:
            friend class cereal::access;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(hasAlbedo, hasNormal, hasMetallic, hasRoughness, hasAO, hasEmissive);
            }
    };

    /**
     * @brief Class representing a material.
     */
    class Material : public Resource
    {
    public:

        Material();
        /**
         * @brief Default constructor for the Material class.
         */
        Material(const std::string& name);

        /**
         * @brief Constructs a Material with the specified shader.
         * @param shader The shader to be used with the material.
         */
        Material(const std::string& name, Ref<Shader> shader);

        /**
         * @brief Constructs a Material from a file path.
         * @param path The file path to the material definition.
         * @note This constructor is for future use when the material YAML exists.
         */
        Material(std::filesystem::path& path);

        /**
         * @brief Constructs a Material with the specified textures.
         * @param materialTextures The textures to be used with the material.
         */
        Material(const std::string& name, MaterialTextures& materialTextures);

        Material(ImportData& importData);

        /**
         * @brief Default destructor for the Material class.
         */
        ~Material() = default;

        /**
         * @brief Uses the material by binding its shader and textures.
         */
        void Use();

        /**
         * @brief Gets the shader associated with the material.
         * @return A reference to the shader.
         */
        Ref<Shader> GetShader() { return m_Shader; }

        MaterialTextures& GetMaterialTextures() { return m_MaterialTextures; }
        MaterialProperties& GetMaterialProperties() { return m_MaterialProperties; }
        MaterialRenderSettings& GetMaterialRenderSettings() { return m_MaterialRenderSettings; }

        //TODO: Remove the materialTextures parameter and make a function that set the materialTextures and the shader too
        static Ref<Material> Create(const std::string& name = "", MaterialTextures* materialTextures = nullptr);

        private:
        
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(m_MaterialTextures, m_MaterialTextureFlags, m_MaterialProperties, m_MaterialRenderSettings, cereal::base_class<Resource>(this));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(m_MaterialTextures, m_MaterialTextureFlags, m_MaterialProperties, m_MaterialRenderSettings, cereal::base_class<Resource>(this));
        }

        template<class Archive>
        static void load_and_construct(Archive& archive, cereal::construct<Material>& construct)
        {
            MaterialTextures materialTextures;
            MaterialTextureFlags materialTextureFlags;
            MaterialProperties materialProperties;
            MaterialRenderSettings materialRenderSettings;
            Resource baseClass;

            archive(materialTextures, materialTextureFlags, materialProperties, materialRenderSettings, cereal::base_class<Resource>(&baseClass));

            construct(baseClass.GetName(), materialTextures);
            construct->m_MaterialTextureFlags = materialTextureFlags;
            construct->m_MaterialProperties = materialProperties;
            construct->m_MaterialRenderSettings = materialRenderSettings;
            construct->m_Name = baseClass.GetName();
            construct->m_FilePath = baseClass.GetPath();
            construct->m_Type = baseClass.GetType();
            construct->m_UUID = baseClass.GetUUID();
        }

    private:
        MaterialTextures m_MaterialTextures; ///< The textures used in the material.
        MaterialTextureFlags m_MaterialTextureFlags; ///< The flags for the textures used in the material.
        MaterialProperties m_MaterialProperties; ///< The properties of the material.
        MaterialRenderSettings m_MaterialRenderSettings; ///< The render settings of the material.
        Ref<Shader> m_Shader; ///< The shader used with the material.
        static Ref<Texture2D> s_MissingTexture; ///< The texture to use when a texture is missing.
        static Ref<Shader> s_StandardShader; ///< The standard shader to use with the material. (When the material be a base class of PBRMaterial and ShaderMaterial this should be moved to PBRMaterial)
    };

    /** @} */
}

CEREAL_REGISTER_TYPE(Coffee::Material);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Material);