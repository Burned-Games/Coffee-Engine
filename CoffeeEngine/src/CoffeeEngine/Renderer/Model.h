#pragma once

#include "CoffeeEngine/Animation/AnimationSystem.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/Serialization/GLMSerialization.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/Serialization/GLMSerialization.h"

#include <assimp/scene.h>
#include <cereal/access.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/vector.hpp>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/base/io/archive.h>
#include <ozz/base/io/stream.h>

#include <filesystem>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <string>
#include <vector>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief Class representing a 3D model.
     */
    class Model : public Resource, public std::enable_shared_from_this<Model>
    {
    public:
        /**
         * @brief Default constructor for the Model class.
         */
        Model() : Resource(ResourceType::Model) {};

        /**
         * @brief Constructs a Model from a file path.
         * @param filePath The file path to the model.
         */
        Model(const std::filesystem::path& path);

        Model(ImportData& importData);

        void LoadFromFilePath(const std::filesystem::path& path);

        /**
         * @brief Gets the meshes of the model.
         * @return A reference to the vector of meshes.
         */
        const std::vector<Ref<Mesh>>& GetMeshes() const { return m_Meshes; };

        /**
         * @brief Adds a mesh to the model.
         * @param mesh A reference to the mesh to add.
         */
        void AddMesh(const Ref<Mesh> mesh) { m_Meshes.push_back(mesh); };

        /**
         * @brief Gets the name of the node.
         * @return The name of the node.
         */
        const std::string& GetNodeName() { return m_NodeName; }

        /**
         * @brief Gets the parent model.
         * @return A pointer to the parent model.
         */
        const std::weak_ptr<Model> GetParent() const { return m_Parent; }

        /**
         * @brief Gets the children models.
         * @return A reference to the vector of children models.
         */
        const std::vector<Ref<Model>> GetChildren() const { return m_Children; }

        /**
         * @brief Gets the transformation matrix of the model.
         * @return The transformation matrix.
         */
        const glm::mat4 GetTransform() const { return m_Transform; }

        /**
        * @brief Loads a model from the specified file path.
        * @param path The path to the model file.
        * @return A reference to the loaded model.
        */
        static Ref<Model> Load(const std::filesystem::path& path);

        /**
         * @brief Checks if the model has animations.
         * @return True if the model has animations, false otherwise.
         */
        bool HasAnimations() const { return m_hasAnimations; }

        /**
         * @brief Gets the skeleton of the model.
         * @return A reference to the skeleton.
         */
        const Ref<Skeleton>& GetSkeleton() const { return m_Skeleton; };

        /**
         * @brief Gets the animation controller of the model.
         * @return A reference to the animation controller.
         */
        const Ref<AnimationController>& GetAnimationController() const { return m_AnimationController; };

        /**
         * @brief Saves the animations of the model.
         * @param uuid The UUID of the model.
         */
        void SaveAnimations(const UUID uuid) const;

        /**
         * @brief Imports animations for the model.
         * @param uuid The UUID of the model.
         */
        void ImportAnimations(const UUID uuid);

    private:
        /**
         * @brief Processes a mesh from the Assimp mesh and scene.
         * @param mesh The Assimp mesh.
         * @param scene The Assimp scene.
         * @param joints The joints.
         * @param boneMap The bone map.
         * @return A reference to the processed mesh.
         */
        Ref<Mesh> processMesh(aiMesh* mesh, const aiScene* scene, std::vector<Joint>& joints, std::map<std::string, int>& boneMap);

        /**
         * @brief Processes a node from the Assimp node and scene.
         * @param node The Assimp node.
         * @param scene The Assimp scene.
         * @param joints The joints.
         * @param boneMap The bone map.
         */
        void processNode(aiNode* node, const aiScene* scene, std::vector<Joint>& joints, std::map<std::string, int>& boneMap);

        /**
         * @brief Loads a texture from the Assimp material and texture type.
         * @param material The Assimp material.
         * @param type The Assimp texture type.
         * @return A reference to the loaded texture.
         */
        Ref<Texture2D> LoadTexture2D(aiMaterial* material, aiTextureType type);

        /**
         * @brief Loads material textures from the Assimp material.
         * @param material The Assimp material.
         * @return The loaded material textures.
         */
        PBRMaterialTextures LoadMaterialTextures(aiMaterial* material);
        
        friend class cereal::access;
        template<class Archive>
        void save(Archive& archive) const
        {
            // convert this to UUIDs
            std::vector<UUID> meshUUIDs;
            for(const auto& mesh : m_Meshes)
            {
                meshUUIDs.push_back(mesh->GetUUID());
            }
            archive(meshUUIDs, m_Parent, m_Children, m_Transform, m_NodeName, m_hasAnimations, m_AnimationsNames, m_Joints, cereal::base_class<Resource>(this));
            SaveAnimations(m_UUID);
        }
        template<class Archive>
        void load(Archive& archive)
        {
            std::vector<UUID> meshUUIDs;
            archive(meshUUIDs, m_Parent, m_Children, m_Transform, m_NodeName, m_hasAnimations, m_AnimationsNames, m_Joints, cereal::base_class<Resource>(this));
            for (const auto& data : meshUUIDs)
            {
                m_Meshes.push_back(ResourceLoader::GetResource<Mesh>(data));
            }
            ImportAnimations(m_UUID);
        }

        /**
         * @brief Converts an Assimp matrix to a GLM matrix.
         * @param from The Assimp matrix.
         * @return The GLM matrix.
         */
        static glm::mat4 AiToGlmMat4(const aiMatrix4x4& from)
        {
            glm::mat4 to;
            memcpy(glm::value_ptr(to), &from, sizeof(glm::mat4));
            return glm::transpose(to);
        }

        /**
         * @brief Converts an Assimp transform to an Ozz transform.
         * @param aiTransform The Assimp transform.
         * @return The Ozz transform.
         */
        static ozz::math::Transform AiToOzzTransform(const aiMatrix4x4& aiTransform)
        {
            aiVector3D scale, translation;
            aiQuaternion rotation;
            aiTransform.Decompose(scale, rotation, translation);

            ozz::math::Transform ozzTransform = {
                ozz::math::Float3(translation.x, translation.y, translation.z),
                ozz::math::Quaternion(rotation.x, rotation.y, rotation.z, rotation.w),
                ozz::math::Float3(scale.x, scale.y, scale.z)
            };
            return ozzTransform;
        }

        unsigned int MAX_BONE_INFLUENCE = 4;

        /**
         * @brief Extracts the skeleton from the Assimp scene.
         * @param pScene The Assimp scene.
         * @param joints The joints.
         * @param boneMap The bone map.
         * @return True if the skeleton was extracted successfully, false otherwise.
         */
        bool ExtractSkeleton(const aiScene* pScene, std::vector<Joint>& joints, std::map<std::string, int>& boneMap);

        /**
         * @brief Extracts animations from the Assimp scene.
         * @param scene The Assimp scene.
         * @param boneMap The bone map.
         * @return True if the animations were extracted successfully, false otherwise.
         */
        bool ExtractAnimations(const aiScene* scene, const std::map<std::string, int>& boneMap);

        /**
         * @brief Extracts joints from the Assimp node.
         * @param node The Assimp node.
         * @param parentIndex The index of the parent joint.
         * @param joints The joints.
         * @param boneMap The bone map.
         */
        void ExtractJoints(const aiNode* node, int parentIndex, std::vector<Joint>& joints, std::map<std::string, int>& boneMap);

        /**
         * @brief Processes an animation channel.
         * @param channel The Assimp animation channel.
         * @param track The Ozz joint track.
         * @param ticksPerSecond The ticks per second.
         */
        void ProcessAnimationChannel(aiNodeAnim* channel, ozz::animation::offline::RawAnimation::JointTrack& track, float ticksPerSecond);

    private:
        std::vector<Ref<Mesh>> m_Meshes; ///< The meshes of the model.

        std::weak_ptr<Model> m_Parent; ///< The parent model.
        std::vector<Ref<Model>> m_Children; ///< The children models.

        glm::mat4 m_Transform; ///< The transformation matrix of the model.

        std::string m_NodeName; ///< The name of the node.

        bool m_hasAnimations = false; ///< Indicates if the model has animations.
        Ref<Skeleton> m_Skeleton; ///< The skeleton of the model.
        Ref<AnimationController> m_AnimationController; ///< The animation controller of the model.

        std::vector<std::string> m_AnimationsNames; ///< The names of the animations.
        std::vector<Joint> m_Joints; ///< The joints of the model.
    };

    /** @} */
}

CEREAL_REGISTER_TYPE(Coffee::Model);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Model);