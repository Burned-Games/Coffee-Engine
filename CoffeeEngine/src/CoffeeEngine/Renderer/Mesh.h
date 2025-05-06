#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/Renderer/Buffer.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/VertexArray.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include "CoffeeEngine/IO/Serialization/GLMSerialization.h"

#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <array>
#include <cereal/access.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/polymorphic.hpp>

namespace Coffee {

    struct MeshImportData;

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief Structure representing a vertex in a mesh.
     */
    struct Vertex {
        glm::vec3 Position = glm::vec3(0.0f); ///< The position of the vertex.
        glm::vec2 TexCoords = glm::vec2(0.0f); ///< The texture coordinates of the vertex.
        glm::vec3 Normals = glm::vec3(0.0f); ///< The normal vector of the vertex.
        glm::vec3 Tangent = glm::vec3(0.0f); ///< The tangent vector of the vertex.
        glm::vec3 Bitangent = glm::vec3(0.0f); ///< The bitangent vector of the vertex.
        glm::ivec4 BoneIDs = glm::ivec4(-1); ///< The bone IDs of the vertex.
        glm::vec4 BoneWeights = glm::vec4(0.0f); ///< The bone weights of the vertex.

        private:
            friend class cereal::access;

            template<class Archive>
            void serialize(Archive& archive)
            {
                archive(Position, TexCoords, Normals, Tangent, Bitangent, BoneIDs, BoneWeights);
            }
    };

    /**
     * @brief Class representing a mesh.
     */
    class Mesh : public Resource
    {
    public:
        /**
         * @brief Constructs a Mesh with the specified indices and vertices.
         * @param indices The indices of the mesh.
         * @param vertices The vertices of the mesh.
         */
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
        Mesh(const ImportData& importData);

        /**
         * @brief Gets the vertex array of the mesh.
         * @return A reference to the vertex array.
         */
        const Ref<VertexArray>& GetVertexArray() const { return m_VertexArray; }

        /**
         * @brief Gets the vertex buffer of the mesh.
         * @return A reference to the vertex buffer.
         */
        const Ref<VertexBuffer>& GetVertexBuffer() const { return m_VertexBuffer; }

        /**
         * @brief Gets the index buffer of the mesh.
         * @return A reference to the index buffer.
         */
        const Ref<IndexBuffer>& GetIndexBuffer() const { return m_IndexBuffer; }

        /**
         * @brief Sets the material of the mesh.
         * @param material A reference to the material.
         */
        void SetMaterial(Ref<Material>& material) { m_Material = material; }

        /**
         * @brief Sets the axis-aligned bounding box (AABB) of the mesh.
         * @param aabb The axis-aligned bounding box to set.
         */
        void SetAABB(const AABB aabb) { m_AABB = aabb; }

        /**
         * @brief Gets the axis-aligned bounding box (AABB) of the mesh.
         * @return A reference to the AABB.
         */
        const AABB& GetAABB() { return m_AABB; }

        /**
         * @brief Gets the oriented bounding box (OBB) of the mesh.
         * @param transform The transformation matrix.
         * @return The OBB.
         */
        OBB GetOBB(const glm::mat4& transform) { return {transform, GetAABB()}; }

        /**
         * @brief Gets the material of the mesh.
         * @return A reference to the material.
         */
        const Ref<Material>& GetMaterial() const { return m_Material; }

        /**
         * @brief Gets the vertices of the mesh.
         * @return A reference to the vector of vertices.
         */
        const std::vector<Vertex>& GetVertices() const { return m_Vertices; }

        /**
         * @brief Gets the indices of the mesh.
         * @return A reference to the vector of indices.
         */
        const std::vector<uint32_t>& GetIndices() const { return m_Indices; }

    private:
        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            UUID materialUUID = m_Material->GetUUID();
            archive(m_Vertices, m_Indices, m_AABB, materialUUID, cereal::base_class<Resource>(this));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            UUID materialUUID;
            archive(m_Vertices, m_Indices, m_AABB, materialUUID, cereal::base_class<Resource>(this));

            m_Material = ResourceLoader::GetResource<PBRMaterial>(materialUUID);
        }

        template<class Archive>
        static void load_and_construct(Archive& data, cereal::construct<Mesh>& construct)
        {
            // Try to take this data as a reference
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;
            data(vertices, indices);
            construct(vertices, indices);

            UUID materialUUID;

            data(construct->m_AABB, materialUUID, cereal::base_class<Resource>(construct.ptr()));
            construct->m_Vertices = vertices;
            construct->m_Indices = indices;
            construct->m_Material = ResourceLoader::GetResource<PBRMaterial>(materialUUID);
        }
      private:
        Ref<VertexArray> m_VertexArray; ///< The vertex array of the mesh.
        Ref<VertexBuffer> m_VertexBuffer; ///< The vertex buffer of the mesh.
        Ref<IndexBuffer> m_IndexBuffer; ///< The index buffer of the mesh.

        Ref<Material> m_Material; ///< The material of the mesh.
        AABB m_AABB; ///< The axis-aligned bounding box of the mesh.

        std::vector<uint32_t> m_Indices; ///< The indices of the mesh.
        std::vector<Vertex> m_Vertices; ///< The vertices of the mesh.
    };

    /** @} */
}

CEREAL_REGISTER_TYPE(Coffee::Mesh);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Mesh);