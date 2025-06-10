#pragma once

#include "CoffeeEngine/Renderer/Mesh.h"

#include <memory>
#include <vector>

/**
 * @brief Structure representing a triangle in the navigation mesh.
 */
struct NavMeshTriangle
{
    glm::vec3 vertices[3]; ///< Vertices of the triangle
    glm::vec3 center; ///< Center of the triangle
    glm::vec3 normal; ///< Normal of the triangle
    std::vector<int> neighbors; ///< Indices of neighboring triangles

    template<class Archive> void serialize(Archive& archive, std::uint32_t const version)
    {
        archive(cereal::make_nvp("Vertices", vertices), cereal::make_nvp("Center", center), cereal::make_nvp("Normal", normal), cereal::make_nvp("Neighbors", neighbors));
    }
};

namespace Coffee
{
    /**
     * @brief Class representing a navigation mesh.
     */
    class NavMesh
    {
    public:
        /**
         * @brief Constructor.
         */
        NavMesh() : WalkableSlopeAngle(45.0f) {}

        /**
         * @brief Destructor.
         */
        ~NavMesh() { Clear(); }

        /**
         * @brief Calculates walkable areas from the given mesh and world transform.
         * @param mesh The mesh to process.
         * @param worldTransform The world transform of the mesh.
         * @return True if the walkable areas were successfully calculated, false otherwise.
         */
        bool CalculateWalkableAreas(const std::shared_ptr<Mesh>& mesh, const glm::mat4& worldTransform);

        /**
         * @brief Renders the walkable areas for debugging purposes.
         */
        void RenderWalkableAreas() const;

        /**
         * @brief Clears the navigation mesh data.
         */
        void Clear();

        /**
         * @brief Returns the triangles in the navigation mesh.
         * @return A reference to the vector of triangles.
         */
        const std::vector<NavMeshTriangle>& GetTriangles() const { return m_Triangles; }

        /**
         * @brief Checks if the navigation mesh has been calculated.
         * @return True if the navigation mesh has been calculated, false otherwise.
         */
        bool IsCalculated() const { return m_Calculated; }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            std::ostringstream ss;
            cereal::BinaryOutputArchive outputArchive(ss);
            outputArchive(m_Triangles, m_Calculated, WalkableSlopeAngle);

            std::string binData = ss.str();
            archive(cereal::make_nvp("NavMesh", cereal::base64::encode(reinterpret_cast<const unsigned char*>(binData.data()), binData.size())));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            std::string serializedData;
            archive(cereal::make_nvp("NavMesh", serializedData));

            std::string decoded = cereal::base64::decode(serializedData);
            std::istringstream ss(decoded);
            cereal::BinaryInputArchive inputArchive(ss);
            inputArchive(m_Triangles, m_Calculated, WalkableSlopeAngle);
        }
    public:
        float WalkableSlopeAngle; ///< Maximum walkable slope angle

    private:
        /**
         * @brief Processes the mesh data to generate navigation mesh triangles.
         * @param vertices The vertices of the mesh.
         * @param indices The indices of the mesh.
         * @param transform The transformation matrix.
         */
        void ProcessMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const glm::mat4& transform);

        /**
         * @brief Calculates the neighbors for each triangle in the navigation mesh.
         */
        void CalculateNeighbors();

        /**
         * @brief Checks if two triangles are adjacent.
         * @param triA The index of the first triangle.
         * @param triB The index of the second triangle.
         * @return True if the triangles are adjacent, false otherwise.
         */
        bool AreTrianglesAdjacent(int triA, int triB) const;

        /**
         * @brief Checks if a surface is walkable based on its normal.
         * @param normal The normal of the surface.
         * @return True if the surface is walkable, false otherwise.
         */
        bool IsWalkableSurface(const glm::vec3& normal) const;

    private:
        std::vector<NavMeshTriangle> m_Triangles; ///< Triangles in the navigation mesh
        bool m_Calculated = false; ///< Flag indicating if the navigation mesh has been calculated
    };
} // namespace Coffee
CEREAL_CLASS_VERSION(NavMeshTriangle, 0);
CEREAL_CLASS_VERSION(Coffee::NavMesh, 0);