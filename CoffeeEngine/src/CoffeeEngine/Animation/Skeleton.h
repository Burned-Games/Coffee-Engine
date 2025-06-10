#pragma once

#include <ozz/animation/runtime/skeleton.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>
#include <glm/mat4x4.hpp>

#include <string>
#include <vector>

namespace cereal {
    class access;
}

namespace Coffee {

    /**
     * @brief Struct representing a joint.
     */
    struct Joint
    {
        std::string name; ///< The name of the joint.
        int parentIndex; ///< The index of the parent joint.
        ozz::math::Transform localTransform; ///< The local transform of the joint.
        glm::mat4 invBindPose; ///< The inverse bind pose matrix of the joint.

    private:
        friend class cereal::access;

        /**
         * @brief Serializes the Joint.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template<class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(name, parentIndex);

            archive(localTransform.translation.x,
                   localTransform.translation.y,
                   localTransform.translation.z);

            archive(localTransform.rotation.x,
                   localTransform.rotation.y,
                   localTransform.rotation.z,
                   localTransform.rotation.w);

            archive(localTransform.scale.x,
                   localTransform.scale.y,
                   localTransform.scale.z);

            archive(invBindPose);
        }
    };

    /**
     * @brief Class representing a skeleton.
     */
    class Skeleton
    {
    public:
        Skeleton() = default;
        ~Skeleton() = default;

        /**
         * @brief Sets the skeleton.
         * @param skeleton The skeleton to set.
         */
        void SetSkeleton(ozz::unique_ptr<ozz::animation::Skeleton> skeleton);

        /**
         * @brief Gets the skeleton.
         * @return The skeleton.
         */
        const ozz::animation::Skeleton* GetSkeleton() const { return m_Skeleton.get(); }

        /**
         * @brief Sets the joints.
         * @param joints The joints to set.
         */
        void SetJoints(const std::vector<Joint>& joints);

        /**
         * @brief Gets the joints.
         * @return The joints.
         */
        const std::vector<Joint>& GetJoints() const { return m_Joints; }

        /**
         * @brief Gets the number of joints.
         * @return The number of joints.
         */
        unsigned int GetNumJoints() const { return m_NumJoints; }

        /**
         * @brief Gets the joint matrices.
         * @return The joint matrices.
         */
        const std::vector<glm::mat4>& GetJointMatrices() const { return m_JointMatrices; }

        /**
         * @brief Saves the skeleton to an archive.
         * @param archive The archive to save to.
         */
        void Save(ozz::io::OArchive& archive) const;

        /**
         * @brief Loads the skeleton from an archive.
         * @param archive The archive to load from.
         * @param joints The joints to load.
         */
        void Load(ozz::io::IArchive& archive, std::vector<Joint>& joints);

    private:
        ozz::unique_ptr<ozz::animation::Skeleton> m_Skeleton; ///< The skeleton.
        std::vector<Joint> m_Joints; ///< The joints.
        unsigned int m_NumJoints = 0; ///< The number of joints.
        std::vector<glm::mat4> m_JointMatrices; ///< The joint matrices.
    };

} // Coffee
