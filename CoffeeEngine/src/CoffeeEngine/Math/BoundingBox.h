#pragma once

#include <glm/glm.hpp>
#include <cereal/access.hpp>

#include <array>

namespace Coffee {

    enum class IntersectionType {
        Outside,
        Inside,
        Intersect
    };

    /**
     * @brief Structure representing an axis-aligned bounding box (AABB).
     */
    struct AABB {

        glm::vec3 min = glm::vec3(0.0f); ///< The minimum point of the AABB.
        glm::vec3 max = glm::vec3(0.0f); ///< The maximum point of the AABB.

        AABB() = default;

        /**
         * @brief Constructs an AABB with specified minimum and maximum points.
         * @param min The minimum point of the AABB.
         * @param max The maximum point of the AABB.
         */
        AABB(const glm::vec3& min, const glm::vec3& max)
            : min(min), max(max) {}

        glm::vec3 GetCenter() const
        {
            return (min + max) / 2.0f;
        }

        glm::vec3 GetHalfSize() const
        {
            return (max - min) / 2.0f;
        }

        bool IsValid() const
        {
            return min.x < max.x && min.y < max.y && min.z < max.z;
        }

        AABB CalculateTransformedAABB(const glm::mat4& transform) const
        {
            AABB aabb = *this;

            // Compute the 8 corners of the AABB
            glm::vec3 corners[8] = {
                aabb.min,
                glm::vec3(aabb.min.x, aabb.min.y, aabb.max.z),
                glm::vec3(aabb.min.x, aabb.max.y, aabb.min.z),
                glm::vec3(aabb.min.x, aabb.max.y, aabb.max.z),
                glm::vec3(aabb.max.x, aabb.min.y, aabb.min.z),
                glm::vec3(aabb.max.x, aabb.min.y, aabb.max.z),
                glm::vec3(aabb.max.x, aabb.max.y, aabb.min.z),
                aabb.max
            };

            // Transform the corners
            glm::vec3 transformedCorners[8];
            for (int i = 0; i < 8; ++i) {
                transformedCorners[i] = glm::vec3(transform * glm::vec4(corners[i], 1.0f));
            }

            // Find the new min and max points
            glm::vec3 newMin = transformedCorners[0];
            glm::vec3 newMax = transformedCorners[0];

            for (int i = 1; i < 8; ++i) {
                newMin = glm::min(newMin, transformedCorners[i]);
                newMax = glm::max(newMax, transformedCorners[i]);
            }

            // Create the transformed AABB
            AABB transformedAABB(newMin, newMax);

            return transformedAABB;
        }

        // Used when the AABB's min and max points are in local space of the object
        IntersectionType Intersect(const AABB& other, const glm::mat4& thisTransform, const glm::mat4& otherTransform) const {
            // Transform the other AABB to the current AABB's local space
            glm::mat4 inverseThisTransform = glm::inverse(thisTransform);
            glm::mat4 relativeTransform = inverseThisTransform * otherTransform;
            AABB transformedOther = other.CalculateTransformedAABB(relativeTransform);

            // Perform the intersection test in local space
            if (max.x < transformedOther.min.x || min.x > transformedOther.max.x) return IntersectionType::Outside;
            if (max.y < transformedOther.min.y || min.y > transformedOther.max.y) return IntersectionType::Outside;
            if (max.z < transformedOther.min.z || min.z > transformedOther.max.z) return IntersectionType::Outside;

            if (min.x < transformedOther.min.x && max.x > transformedOther.max.x &&
                min.y < transformedOther.min.y && max.y > transformedOther.max.y &&
                min.z < transformedOther.min.z && max.z > transformedOther.max.z) return IntersectionType::Inside;

            return IntersectionType::Intersect;
        }

        // Used when the AABB's min and max points are in world space
        IntersectionType Intersect(const AABB& other) const {
            if (max.x < other.min.x || min.x > other.max.x) return IntersectionType::Outside;
            if (max.y < other.min.y || min.y > other.max.y) return IntersectionType::Outside;
            if (max.z < other.min.z || min.z > other.max.z) return IntersectionType::Outside;

            if (min.x < other.min.x && max.x > other.max.x &&
                min.y < other.min.y && max.y > other.max.y &&
                min.z < other.min.z && max.z > other.max.z) return IntersectionType::Inside;

            return IntersectionType::Intersect;
        }

        // Used when the AABB's min and max points are in local space of the object
        IntersectionType Intersect(const glm::mat4& thisTransform, const glm::vec3& point) const {
            const AABB& transformedAABB = CalculateTransformedAABB(thisTransform);

            if (point.x < transformedAABB.min.x || point.x > transformedAABB.max.x) return IntersectionType::Outside;
            if (point.y < transformedAABB.min.y || point.y > transformedAABB.max.y) return IntersectionType::Outside;
            if (point.z < transformedAABB.min.z || point.z > transformedAABB.max.z) return IntersectionType::Outside;

            return IntersectionType::Inside;
        }

        // Used when the AABB's min and max points are in world space
        IntersectionType Intersect(const glm::vec3& point) const {
            if (point.x < min.x || point.x > max.x) return IntersectionType::Outside;
            if (point.y < min.y || point.y > max.y) return IntersectionType::Outside;
            if (point.z < min.z || point.z > max.z) return IntersectionType::Outside;

            return IntersectionType::Inside;
        }

        private:
            friend class cereal::access;

            template<class Archive> void serialize(Archive& archive, std::uint32_t const version)
            {
                archive(min, max);
            }
    };

    /**
     * @brief Structure representing an oriented bounding box (OBB).
     */
    struct OBB
    {
        std::array<glm::vec3, 8> corners; ///< The corners of the OBB.

        OBB() = default;

        /**
         * @brief Constructs an OBB with specified corners.
         * @param corners The corners of the OBB.
         */
        OBB(const std::array<glm::vec3, 8>& corners)
            : corners(corners) {}

        /**
         * @brief Constructs an OBB from a transformation matrix and an AABB.
         * @param transform The transformation matrix.
         * @param aabb The axis-aligned bounding box.
         */
        OBB(const glm::mat4& transform, const AABB& aabb)
            : corners({
                glm::vec3(transform * glm::vec4(aabb.min.x, aabb.min.y, aabb.min.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.max.x, aabb.min.y, aabb.min.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.max.x, aabb.max.y, aabb.min.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.min.x, aabb.min.y, aabb.max.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.max.x, aabb.max.y, aabb.max.z, 1.0f)),
                glm::vec3(transform * glm::vec4(aabb.min.x, aabb.max.y, aabb.max.z, 1.0f))
                }) {}

        bool IsValid() const
        {
            return corners[0].x <= corners[1].x && corners[0].y <= corners[2].y && corners[0].z <= corners[4].z;
        }
    };

}