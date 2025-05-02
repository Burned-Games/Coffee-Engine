/**
 * @defgroup io IO
 * @brief IO components of the CoffeeEngine.
 * @{
 */

#pragma once
#include "CoffeeEngine/Core/Assert.h"
#include "CoffeeEngine/Core/UUID.h"
#include "CoffeeEngine/IO/Serialization/FilesystemPathSerialization.h"
#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>
#include <filesystem>

#include <cereal/types/polymorphic.hpp>

namespace Coffee {

    /**
     * @enum ResourceType
     * @brief Enumeration of different types of resources.
     */
    enum class ResourceType
    {
        Unknown, ///< Unknown resource type
        Texture, ///< Texture resource type
        Texture2D, ///< Texture2D resource type
        Cubemap, ///< Cubemap resource type
        Model,   ///< Model resource type
        Mesh,   ///< Mesh resource type
        Shader,   ///< Shader resource type
        Material, ///< Material resource type
        PBRMaterial, ///< PBRMaterial resource type
        ShaderMaterial, ///< ShaderMaterial resource type
        AnimationSystem, ///< AnimationSystem resource type
        Skeleton, ///< Skeleton resource type
        Animation, ///< Animation resource type
        AnimationController, ///< AnimationController resource type
        Prefab, ///< Prefab resource type
    };

    /**
     * @class Resource
     * @brief Base class for different types of resources in the CoffeeEngine.
     */
    class Resource
    {
    public:
        /**
         * @brief Constructor that initializes the resource with a type.
         * @param type The type of the resource.
         */
        Resource(ResourceType type = ResourceType::Unknown)
            : m_Type(type) {}

        /**
         * @brief Virtual destructor.
         */
        virtual ~Resource() = default;

        /**
         * @brief Gets the name of the resource.
         * @return The name of the resource.
         */
        const std::string& GetName() const { return m_Name; }

        /**
         * @brief Gets the file path of the resource.
         * @return The file path of the resource.
         */
        const std::filesystem::path& GetPath() { COFFEE_CORE_ASSERT(m_FilePath.empty(), "This Texture does not exist on disk!"); return m_FilePath; }

        void SetPath(const std::filesystem::path& path) { m_FilePath = path; }

        /**
         * @brief Sets the name of the mesh.
         * @param name The name of the mesh.
         */
        void SetName(const std::string& name) { m_Name = name; }

        /**
         * @brief Gets the type of the resource.
         * @return The type of the resource.
         */
        ResourceType GetType() const { return m_Type; }

        /**
         * @brief Sets the UUID of the resource.
         * @param uuid The UUID to set.
         */
        void SetUUID(UUID uuid) { m_UUID = uuid; }

        /**
         * @brief Gets the UUID of the resource.
         * @return The UUID of the resource.
         */
        UUID GetUUID() const { return m_UUID; }

        /**
         * @brief Sets the embedded status of the resource.
         * @param isEmbedded The embedded status to set.
         */
        void SetEmbedded(bool isEmbedded) { m_isEmbedded = isEmbedded; }

        /**
         * @brief Checks if the resource is embedded.
         * @return True if the resource is embedded, false otherwise.
         */
        bool IsEmbedded() const { return m_isEmbedded; }

    private:
        friend class cereal::access;

        /**
         * @brief Serializes the resource to an archive.
         * @tparam Archive The type of the archive.
         * @param archive The archive to save the resource to.
         */
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            int typeInt = static_cast<int>(m_Type);
            archive(m_Name, m_FilePath, typeInt, m_UUID, m_isEmbedded);
        }

        /**
         * @brief Deserializes the resource from an archive.
         * @tparam Archive The type of the archive.
         * @param archive The archive to load the resource from.
         */
        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            int typeInt;
            archive(m_Name, m_FilePath, typeInt, m_UUID, m_isEmbedded);
            m_Type = static_cast<ResourceType>(typeInt);
        }

    protected:
        std::string m_Name; ///< The name of the resource.
        std::filesystem::path m_FilePath; ///< The file path of the resource.
        ResourceType m_Type; ///< The type of the resource.
        UUID m_UUID; ///< The UUID of the resource.
        bool m_isEmbedded = false; ///< Flag indicating if the resource is embedded. // TODO: Revise if this is the right place for this
    };

}

CEREAL_REGISTER_TYPE(Coffee::Resource);

/** @} */