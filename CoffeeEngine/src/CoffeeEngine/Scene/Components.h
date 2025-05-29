/**
 * @defgroup scene Scene
 * @{
 */

#pragma once

#include "CoffeeEngine//Renderer/Font.h"
#include "CoffeeEngine/Animation/AnimationSystem.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/IO/Serialization/FilesystemPathSerialization.h"
#include "CoffeeEngine/Navigation/NavMesh.h"
#include "CoffeeEngine/Navigation/NavMeshPathfinding.h"
#include "CoffeeEngine/ParticleManager/ParticleManager.h"
#include "CoffeeEngine/Physics/Collider.h"
#include "CoffeeEngine/Physics/RigidBody.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scripting/Script.h"
#include "CoffeeEngine/Scripting/ScriptManager.h"
#include "CoffeeEngine/UI/UIAnchor.h"
#include "CoffeeEngine/UI/UIManager.h"

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <CoffeeEngine/Physics/CollisionCallback.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

#include "CoffeeEngine/Project/Project.h"

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

// FIXME : DONT MOVE THIS INCLUDE
#include "CoffeeEngine/Audio/Audio.h"

namespace Coffee
{
    /**
     * @brief Component representing a tag.
     * @ingroup scene
     */
    struct TagComponent
    {
        std::string Tag; ///< The tag string.

        TagComponent() = default;
        TagComponent(const TagComponent&) = default;
        TagComponent(const std::string& tag) : Tag(tag) {}

        /**
         * @brief Serializes the TagComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("Tag", Tag));
        }
    };

    /**
     * @brief Component representing a transform.
     * @ingroup scene
     */
    struct TransformComponent
    {
      private:
        glm::vec3 Position = {0.0f, 0.0f, 0.0f}; ///< The position vector.
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f}; ///< The rotation vector.
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};    ///< The scale vector.

        glm::mat4 worldMatrix = glm::mat4(1.0f); ///< The world transformation matrix.
        bool isDirty = true;                     ///< Flag to indicate if the transform is dirty.
      public:
        TransformComponent() = default;
        TransformComponent(const TransformComponent&) = default;
        TransformComponent(const glm::vec3& position) : Position(position) {}

        void SetLocalPosition(const glm::vec3& position)
        {
            Position = position;
            isDirty = true; // Mark the transform as dirty
        }

        void SetLocalRotation(const glm::vec3& rotation)
        {
            Rotation = rotation;
            isDirty = true; // Mark the transform as dirty
        }

        void SetLocalScale(const glm::vec3& scale)
        {
            Scale = scale;
            isDirty = true; // Mark the transform as dirty
        }

        /**
         * @brief Gets the local position vector.
         * @return The local position vector.
         */
        glm::vec3& GetLocalPosition() { return Position; }
        glm::vec3& GetLocalRotation() { return Rotation; }
        glm::vec3& GetLocalScale() { return Scale; }

        void SetWorldPosition(const glm::vec3& position)
        {
            Position = position;
            SetWorldTransform(glm::translate(glm::mat4(1.0f), Position) *
                              glm::toMat4(glm::quat(glm::radians(Rotation))) * glm::scale(glm::mat4(1.0f), Scale));
        }

        void SetWorldRotation(const glm::vec3& rotation)
        {
            Rotation = rotation;
            SetWorldTransform(glm::translate(glm::mat4(1.0f), Position) *
                              glm::toMat4(glm::quat(glm::radians(Rotation))) * glm::scale(glm::mat4(1.0f), Scale));
        }

        void SetWorldScale(const glm::vec3& scale)
        {
            Scale = scale;
            SetWorldTransform(glm::translate(glm::mat4(1.0f), Position) *
                              glm::toMat4(glm::quat(glm::radians(Rotation))) * glm::scale(glm::mat4(1.0f), Scale));
        }

        /**
         * @brief Gets the local transformation matrix.
         * @return The local transformation matrix.
         */
        glm::mat4 GetLocalTransform() const
        {
            glm::mat4 rotation = glm::toMat4(glm::quat(glm::radians(Rotation)));

            return glm::translate(glm::mat4(1.0f), Position) * rotation * glm::scale(glm::mat4(1.0f), Scale);
        }

        /**
         * @brief Sets the local transformation matrix.
         * @param transform The transformation matrix to set.
         */
        void SetLocalTransform(
            const glm::mat4& transform) // TODO: Improve this function, this way is ugly and glm::decompose is from gtx
                                        // (is supposed to not be very stable)
        {
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::quat orientation;

            glm::decompose(transform, Scale, orientation, Position, skew, perspective);
            Rotation = glm::degrees(glm::eulerAngles(orientation));
            isDirty = true; // Mark the transform as dirty
        }

        /**
         * @brief Gets the world transformation matrix.
         * @return The world transformation matrix.
         */
        const glm::mat4& GetWorldTransform() const { return worldMatrix; }

        /**
         * @brief Sets the world transformation matrix.
         * @param transform The transformation matrix to set.
         */
        void SetWorldTransform(const glm::mat4& transform)
        {
            worldMatrix = transform * GetLocalTransform();
            isDirty = false; // Mark the transform as clean
        }

        void MarkDirty()
        {
            isDirty = true; // Mark the transform as dirty
        }

        bool IsDirty() const
        {
            return isDirty; // Check if the transform is dirty
        }

        /**
         * @brief Serializes the TransformComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("Position", Position), cereal::make_nvp("Rotation", Rotation),
                    cereal::make_nvp("Scale", Scale));
        }
    };

    /**
     * @brief Component representing a camera.
     * @ingroup scene
     */
    struct CameraComponent
    {
        SceneCamera Camera; ///< The scene camera.

        CameraComponent() = default;
        CameraComponent(const CameraComponent&) = default;

        /**
         * @brief Serializes the CameraComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("Camera", Camera));
        }
    };

    /**
     * @brief Component representing an animator.
     * @ingroup scene
     */
    struct AnimatorComponent
    {
        AnimatorComponent() = default;

        /**
         * @brief Copy constructor for AnimatorComponent.
         * @param other The other AnimatorComponent to copy from.
         */
        AnimatorComponent(const AnimatorComponent& other)
            : Loop(other.Loop), BlendDuration(other.BlendDuration), AnimationSpeed(other.AnimationSpeed),
              JointMatrices(other.JointMatrices), modelUUID(other.modelUUID), animatorUUID(other.animatorUUID),
              m_Skeleton(other.m_Skeleton), m_AnimationController(other.m_AnimationController),
              UpperAnimation(other.UpperAnimation), LowerAnimation(other.LowerAnimation),
              PartialBlendThreshold(other.PartialBlendThreshold), UpperBodyWeight(other.UpperBodyWeight),
              LowerBodyWeight(other.LowerBodyWeight), UpperBodyRootJoint(other.UpperBodyRootJoint)
        {
            m_BlendJob.layers = ozz::make_span(m_BlendLayers);
            const std::string rootJointName = GetSkeleton()->GetJoints()[UpperBodyRootJoint].name;
            AnimationSystem::SetupPartialBlending(UpperAnimation->CurrentAnimation, LowerAnimation->CurrentAnimation,
                                                  rootJointName, this);
            AnimationSystem::AddAnimator(this);
        }

        /**
         * @brief Constructs an AnimatorComponent with the given skeleton, animation controller, and animation system.
         * @param skeleton The skeleton reference.
         * @param animationController The animation controller reference.
         */
        AnimatorComponent(Ref<Skeleton> skeleton, Ref<AnimationController> animationController)
            : m_Skeleton(std::move(skeleton)), m_AnimationController(std::move(animationController)),
              UpperAnimation(std::make_shared<AnimationLayer>()), LowerAnimation(std::make_shared<AnimationLayer>())
        {
            m_BlendJob.layers = ozz::make_span(m_BlendLayers);
            JointMatrices = m_Skeleton->GetJointMatrices();
        }

        /**
         * @brief Gets the skeleton reference.
         * @return The skeleton reference.
         */
        Ref<Skeleton> GetSkeleton() const { return m_Skeleton; }

        /**
         * @brief Sets the skeleton reference.
         * @param skeleton The skeleton reference to set.
         */
        void SetSkeleton(Ref<Skeleton> skeleton) { m_Skeleton = std::move(skeleton); }

        /**
         * @brief Gets the animation controller reference.
         * @return The animation controller reference.
         */
        Ref<AnimationController> GetAnimationController() const { return m_AnimationController; }

        /**
         * @brief Sets the animation controller reference.
         * @param animationController The animation controller reference to set.
         */
        void SetAnimationController(Ref<AnimationController> animationController)
        {
            m_AnimationController = std::move(animationController);
        }

        /**
         * @brief Gets the sampling job context.
         * @return The sampling job context.
         */
        ozz::animation::SamplingJob::Context& GetContext() { return m_Context; }

        /**
         * @brief Gets the blend layers.
         * @return The blend layers.
         */
        ozz::animation::BlendingJob::Layer* GetBlendLayers() { return m_BlendLayers; }

        /**
         * @brief Gets the blending job.
         * @return The blending job.
         */
        ozz::animation::BlendingJob& GetBlendJob() { return m_BlendJob; }

        /**
         * @brief Sets the current animation for both upper and lower body layers.
         * @param index The index of the animation to set.
         */
        void SetCurrentAnimation(unsigned int index)
        {
            AnimationSystem::SetCurrentAnimation(index, this, UpperAnimation.get());
            AnimationSystem::SetCurrentAnimation(index, this, LowerAnimation.get());
        }

        /**
         * @brief Sets the current animation for the upper body layer.
         * @param index The index of the animation to set.
         */
        void SetUpperAnimation(unsigned int index)
        {
            AnimationSystem::SetCurrentAnimation(index, this, UpperAnimation.get());
        }

        /**
         * @brief Sets the current animation for the lower body layer.
         * @param index The index of the animation to set.
         */
        void SetLowerAnimation(unsigned int index)
        {
            AnimationSystem::SetCurrentAnimation(index, this, LowerAnimation.get());
        }

        /**
         * @brief Serializes the AnimatorComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void save(Archive& archive, const std::uint32_t& version) const
        {
            archive(cereal::make_nvp("BlendDuration", BlendDuration),
                    cereal::make_nvp("AnimationSpeed", AnimationSpeed), cereal::make_nvp("Loop", Loop),
                    cereal::make_nvp("ModelUUID", modelUUID), cereal::make_nvp("AnimatorUUID", animatorUUID),
                    cereal::make_nvp("UpperAnimation", UpperAnimation),
                    cereal::make_nvp("LowerAnimation", LowerAnimation),
                    cereal::make_nvp("PartialBlendThreshold", PartialBlendThreshold),
                    cereal::make_nvp("UpperBodyWeight", UpperBodyWeight),
                    cereal::make_nvp("LowerBodyWeight", LowerBodyWeight),
                    cereal::make_nvp("UpperBodyRootJoint", UpperBodyRootJoint));
        }

        /**
         * @brief Deserializes the AnimatorComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to deserialize from.
         */
        template <class Archive> void load(Archive& archive, const std::uint32_t& version)
        {

            archive(cereal::make_nvp("BlendDuration", BlendDuration),
                    cereal::make_nvp("AnimationSpeed", AnimationSpeed), cereal::make_nvp("Loop", Loop),
                    cereal::make_nvp("ModelUUID", modelUUID), cereal::make_nvp("AnimatorUUID", animatorUUID),
                    cereal::make_nvp("UpperAnimation", UpperAnimation),
                    cereal::make_nvp("LowerAnimation", LowerAnimation),
                    cereal::make_nvp("PartialBlendThreshold", PartialBlendThreshold),
                    cereal::make_nvp("UpperBodyWeight", UpperBodyWeight),
                    cereal::make_nvp("LowerBodyWeight", LowerBodyWeight),
                    cereal::make_nvp("UpperBodyRootJoint", UpperBodyRootJoint));

            AnimationSystem::LoadAnimator(this);
        }

      public:
        bool Loop = true;            ///< Indicates if the animation should loop.
        float BlendDuration = 0.25f; ///< The duration of the blend.
        float AnimationSpeed = 1.0f; ///< The speed of the animation.

        std::vector<glm::mat4> JointMatrices;                    ///< The joint matrices.
        UUID modelUUID;                                          ///< The UUID of the model.
        UUID animatorUUID;                                       ///< The UUID of the animator.
        int UpperBodyRootJoint = 0;                              ///< Index of the root joint for upper body animations.
        std::vector<ozz::math::SoaTransform> PartialBlendOutput; ///< Output transforms for partial blending.

        float UpperBodyWeight = 1.0f;        ///< Weight for blending upper body animations.
        float LowerBodyWeight = 1.0f;        ///< Weight for blending lower body animations.
        float PartialBlendThreshold = 0.01f; ///< Threshold for partial blending.

        Ref<AnimationLayer> UpperAnimation; ///< Animation layer for upper body animations.
        Ref<AnimationLayer> LowerAnimation; ///< Animation layer for lower body animations.

        bool NeedsUpdate = true; ///< Flag to indicate if the animator needs an update.

      private:
        Ref<Skeleton> m_Skeleton;                       ///< The skeleton reference.
        Ref<AnimationController> m_AnimationController; ///< The animation controller reference.

        ozz::animation::SamplingJob::Context m_Context;      ///< The sampling job context.
        ozz::animation::BlendingJob::Layer m_BlendLayers[2]; ///< The blend layers.
        ozz::animation::BlendingJob m_BlendJob;              ///< The blending job.
    };

    /**
     * @brief Component representing a mesh.
     * @ingroup scene
     */
    struct MeshComponent
    {
        Ref<Mesh> mesh;        ///< The mesh reference.
        bool drawAABB = false; ///< Flag to draw the axis-aligned bounding box (AABB).

        AnimatorComponent* animator = nullptr; ///< The animator component.
        UUID animatorUUID = 0;                 ///< The UUID of the animator.

        MeshComponent() {}
        MeshComponent(const MeshComponent&) = default;
        MeshComponent(Ref<Mesh> mesh) : mesh(mesh) {}

        /**
         * @brief Gets the mesh reference.
         * @return The mesh reference.
         */
        const Ref<Mesh>& GetMesh() const { return mesh; }

      private:
        friend class cereal::access;
        /**
         * @brief Serializes the MeshComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("Mesh", mesh->GetUUID()), cereal::make_nvp("AnimatorUUID", animatorUUID));

            if (animator && animatorUUID != 0)
                animator->animatorUUID = animatorUUID;
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID meshUUID;
            archive(cereal::make_nvp("Mesh", meshUUID), cereal::make_nvp("AnimatorUUID", animatorUUID));

            Ref<Mesh> mesh = ResourceRegistry::Get<Mesh>(meshUUID);
            this->mesh = mesh;
        }
    };

    /**
     * @brief Component representing a material.
     * @ingroup scene
     */
    struct MaterialComponent
    {
        Ref<Material> material; ///< The material reference.

        MaterialComponent() {}
        MaterialComponent(const MaterialComponent&) = default;
        MaterialComponent(Ref<Material> material) : material(material) {}

      private:
        friend class cereal::access;
        /**
         * @brief Serializes the MeshComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            if (version < 1)
            {
                archive(cereal::make_nvp("Material", material->GetUUID()));
                return;
            }

            archive(cereal::make_nvp("IsEmbedded", material->IsEmbedded()));
            if (material->IsEmbedded())
            {
                archive(cereal::make_nvp("Material", material));
            }
            else
            {
                archive(cereal::make_nvp("Type", static_cast<int>(material->GetType())));

                if (material->GetType() == ResourceType::PBRMaterial)
                {
                    ResourceSaver::SaveToCache<PBRMaterial>(material->GetUUID(),
                                                            std::dynamic_pointer_cast<PBRMaterial>(material));
                }
                else if (material->GetType() == ResourceType::ShaderMaterial)
                {
                    ResourceSaver::SaveToCache<ShaderMaterial>(material->GetUUID(),
                                                               std::dynamic_pointer_cast<ShaderMaterial>(material));
                }

                archive(cereal::make_nvp("MaterialUUID", material->GetUUID()));
            }
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            if (version < 1)
            {
                UUID materialUUID;
                archive(cereal::make_nvp("Material", materialUUID));

                Ref<Material> material = ResourceLoader::GetResource<Material>(materialUUID);
                this->material = material;
                return;
            }

            bool isEmbedded = false;
            archive(cereal::make_nvp("IsEmbedded", isEmbedded));

            if (isEmbedded)
            {
                archive(cereal::make_nvp("Material", material));
                this->material->SetEmbedded(isEmbedded);
            }
            else
            {
                int typeInt;
                archive(cereal::make_nvp("Type", typeInt));
                ResourceType type = static_cast<ResourceType>(typeInt);

                UUID materialUUID;
                archive(cereal::make_nvp("MaterialUUID", materialUUID));

                if (type == ResourceType::PBRMaterial)
                {
                    Ref<PBRMaterial> material = ResourceLoader::GetResource<PBRMaterial>(materialUUID);
                    this->material = material;
                }
                else if (type == ResourceType::ShaderMaterial)
                {
                    Ref<ShaderMaterial> material = ResourceLoader::GetResource<ShaderMaterial>(materialUUID);
                    this->material = material;
                }
            }
        }
    };

    /**
     * @brief Component representing a light.
     * @ingroup scene
     */
    struct LightComponent
    {
        /**
         * @brief Enum representing the type of light.
         */
        enum Type
        {
            DirectionalLight = 0, ///< Directional light.
            PointLight = 1,       ///< Point light.
            SpotLight = 2         ///< Spot light.
        };

        // Align to 16 bytes(glm::vec4) instead of 12 bytes(glm::vec3) to match the std140 layout in the shader (a vec3
        // is 16 bytes in std140)
        alignas(16) glm::vec3 Color = {1.0f, 1.0f, 1.0f};      ///< The color of the light.
        alignas(16) glm::vec3 Direction = {0.0f, -1.0f, 0.0f}; ///< The direction of the light.
        alignas(16) glm::vec3 Position = {0.0f, 0.0f, 0.0f};   ///< The position of the light.

        float Range = 5.0f;       ///< The range of the light.
        float Attenuation = 1.0f; ///< The attenuation of the light.
        float Intensity = 1.0f;   ///< The intensity of the light.

        float Angle = 45.0f; ///< The angle of the light.

        int type = static_cast<int>(Type::DirectionalLight); ///< The type of the light.

        // Shadows
        bool Shadow = false;
        float ShadowBias = 0.005f;
        float ShadowMaxDistance = 100.0f;

        LightComponent() = default;
        LightComponent(const LightComponent&) = default;

        /**
         * @brief Serializes the LightComponent.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            if (version >= 1)
            {
                archive(cereal::make_nvp("Color", Color), cereal::make_nvp("Direction", Direction),
                        cereal::make_nvp("Position", Position), cereal::make_nvp("Range", Range),
                        cereal::make_nvp("Attenuation", Attenuation), cereal::make_nvp("Intensity", Intensity),
                        cereal::make_nvp("Angle", Angle), cereal::make_nvp("Type", type),
                        cereal::make_nvp("Shadow", Shadow), cereal::make_nvp("ShadowBias", ShadowBias),
                        cereal::make_nvp("ShadowMaxDistance", ShadowMaxDistance));
            }
            else
            {
                archive(cereal::make_nvp("Color", Color), cereal::make_nvp("Direction", Direction),
                        cereal::make_nvp("Position", Position), cereal::make_nvp("Range", Range),
                        cereal::make_nvp("Attenuation", Attenuation), cereal::make_nvp("Intensity", Intensity),
                        cereal::make_nvp("Angle", Angle), cereal::make_nvp("Type", type));
            }
        }
    };

    struct WorldEnvironmentComponent
    {
        // Skybox
        Ref<Cubemap> Skybox; ///< The skybox reference.
        float SkyboxIntensity = 1.0f; ///< The exposure of the skybox.
        
        // Tonemapping
        float TonemappingExposure = 1.0f; ///< The exposure for tonemapping.

        // Mockup
        bool Fog = false;                        ///< Flag to enable fog.
        glm::vec3 FogColor = {0.5f, 0.5f, 0.5f}; ///< The color of the fog.
        float FogDensity = 0.1f;                 ///< The density of the fog.
        float FogHeight = 0.0f; ///< Fog height.
        float FogHeightDensity = 0.1f; ///< Fog height density.///< The end distance of the fog.

        // Mockup
        bool SSAO = false;          ///< Flag to enable screen space ambient occlusion.
        float SSAORadius = 0.5f;    ///< The radius of the SSAO.
        float SSAOStrength = 1.0f;  ///< The strength of the SSAO.
        float SSAOIntensity = 1.0f; ///< The intensity of the SSAO.
        float SSAOScale = 1.0f;     ///< The scale of the SSAO.
        float SSAOBias = 0.1f;      ///< The bias of the SSAO.

        // Mockup
        bool Bloom = false;          ///< Flag to enable bloom.
        float BloomThreshold = 1.0f; ///< The threshold of the bloom.
        float BloomIntensity = 1.0f; ///< The intensity of the bloom.
        float BloomRadius = 1.0f;    ///< The radius of the bloom.

        WorldEnvironmentComponent() = default;
        WorldEnvironmentComponent(const WorldEnvironmentComponent&) = default;


        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            UUID skyboxUUID = Skybox ? Skybox->GetUUID() : UUID::null;
            archive(cereal::make_nvp("Skybox", skyboxUUID));

            archive(cereal::make_nvp("SkyboxIntensity", SkyboxIntensity));
            archive(cereal::make_nvp("TonemappingExposure", TonemappingExposure));
            archive(cereal::make_nvp("Fog", Fog), cereal::make_nvp("FogColor", FogColor),
                    cereal::make_nvp("FogDensity", FogDensity), cereal::make_nvp("FogHeight", FogHeight),
                    cereal::make_nvp("FogHeightDensity", FogHeightDensity));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID skyboxUUID;
            archive(cereal::make_nvp("Skybox", skyboxUUID));
            if(skyboxUUID != UUID::null) this->Skybox = ResourceLoader::GetResource<Cubemap>(skyboxUUID);
            archive(cereal::make_nvp("SkyboxIntensity", SkyboxIntensity));

            if (version >= 1)
            {
                archive(cereal::make_nvp("TonemappingExposure", TonemappingExposure));

                archive(cereal::make_nvp("Fog", Fog), cereal::make_nvp("FogColor", FogColor),
                        cereal::make_nvp("FogDensity", FogDensity), cereal::make_nvp("FogHeight", FogHeight),
                        cereal::make_nvp("FogHeightDensity", FogHeightDensity));
            }
        }
    };

    struct AudioSourceComponent
    {
        uint64_t gameObjectID = 0;      ///< The object ID.
        Ref<Audio::AudioBank> audioBank; ///< The audio bank.
        std::string audioBankName;       ///< The name of the audio bank.
        std::string eventName;           ///< The name of the event.
        float volume = 1.f;              ///< The volume of the audio source.
        bool mute = false;               ///< True if the audio source is muted.
        bool playOnAwake = false;        ///< True if the audio source should play automatically.
        glm::mat4 transform;             ///< The transform of the audio source.
        bool isPlaying = false;          ///< True if the audio source is playing.
        bool isPaused = false;           ///< True if the audio source is paused.
        bool toDelete = false;           ///< True if the audio source should be deleted.
        bool toUnregister = false;       ///< True if the audio source should be unregistered.
        bool toRegister = true;          ///< True if the audio source should be registered.

        AudioSourceComponent() = default;

        AudioSourceComponent(const AudioSourceComponent& other) { *this = other; }

        ~AudioSourceComponent()
        {
            if (isPlaying || playOnAwake)
                Stop();

            if (toUnregister && !toDelete)
                Audio::UnregisterAudioSourceComponent(*this);
        }

        AudioSourceComponent& operator=(const AudioSourceComponent& other)
        {
            if (this != &other)
            {
                gameObjectID = other.gameObjectID;
                audioBank = other.audioBank;
                audioBankName = other.audioBankName;
                eventName = other.eventName;
                volume = other.volume;
                mute = other.mute;
                playOnAwake = other.playOnAwake;
                transform = other.transform;
                isPlaying = other.isPlaying;
                isPaused = other.isPaused;
                toDelete = other.toDelete;
                toUnregister = other.toUnregister;
                toRegister = other.toRegister;

                if (!toDelete)
                {
                    Audio::RegisterAudioSourceComponent(*this);
                    AudioZone::RegisterObject(gameObjectID, transform[3]);
                }
            }
            return *this;
        }

        static AudioSourceComponent CreateCopy(const AudioSourceComponent& other)
        {
            AudioSourceComponent newComp;
            newComp.audioBank = other.audioBank;
            newComp.audioBankName = other.audioBankName;
            newComp.eventName = other.eventName;
            newComp.volume = other.volume;
            newComp.mute = other.mute;
            newComp.playOnAwake = other.playOnAwake;
            newComp.transform = other.transform;

            return newComp;
        }

        void SetVolume(float volumen)
        {
            if (volumen > 1)
            {
                volumen = 1;
            }
            else if (volumen < 0)
            {
                volumen = 0;
            }
            volume = volumen;
            Audio::SetVolume(this->gameObjectID, this->volume);
        }

        void Play() { Audio::PlayEvent(*this); }
        void Stop() { Audio::StopEvent(*this); }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("GameObjectID", gameObjectID), cereal::make_nvp("AudioBank", audioBank),
                    cereal::make_nvp("AudioBankName", audioBankName), cereal::make_nvp("EventName", eventName),
                    cereal::make_nvp("Volume", volume), cereal::make_nvp("Mute", mute),
                    cereal::make_nvp("PlayOnAwake", playOnAwake), cereal::make_nvp("Transform", transform),
                    cereal::make_nvp("ToRegister", toRegister));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("GameObjectID", gameObjectID), cereal::make_nvp("AudioBank", audioBank),
                    cereal::make_nvp("AudioBankName", audioBankName), cereal::make_nvp("EventName", eventName),
                    cereal::make_nvp("Volume", volume), cereal::make_nvp("Mute", mute),
                    cereal::make_nvp("PlayOnAwake", playOnAwake), cereal::make_nvp("Transform", transform));

            if (version >= 1)
            {
                archive(cereal::make_nvp("ToRegister", toRegister));
            }
        }
    };

    struct AudioListenerComponent
    {
        uint64_t gameObjectID = 0; ///< The object ID.
        glm::mat4 transform;        ///< The transform of the audio listener.
        bool toDelete = false;      ///< True if the audio listener should be deleted.
        bool toUnregister = false;  ///< True if the audio listener should be unregistered.
        bool toRegister = true;     ///< True if the audio listener should be registered.

        AudioListenerComponent() = default;

        AudioListenerComponent(const AudioListenerComponent& other) { *this = other; }

        AudioListenerComponent& operator=(const AudioListenerComponent& other)
        {
            if (this != &other)
            {
                gameObjectID = other.gameObjectID;
                transform = other.transform;
                toDelete = other.toDelete;
                toUnregister = other.toUnregister;
                toRegister = other.toRegister;

                if (!toDelete)
                    Audio::RegisterAudioListenerComponent(*this);
            }
            return *this;
        }

        static AudioListenerComponent CreateCopy(const AudioListenerComponent& other)
        {
            AudioListenerComponent newComp;
            newComp.transform = other.transform;

            return newComp;
        }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("GameObjectID", gameObjectID), cereal::make_nvp("Transform", transform),
                    cereal::make_nvp("ToRegister", toRegister));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("GameObjectID", gameObjectID), cereal::make_nvp("Transform", transform));

            if (version >= 1)
            {
                archive(cereal::make_nvp("ToRegister", toRegister));
            }
        }
    };

    struct AudioZoneComponent
    {
        uint64_t zoneID = -1;                 ///< The zone ID.
        std::string audioBusName;             ///< The name of the audio bus.
        glm::vec3 position = {0.f, 0.f, 0.f}; ///< The position of the audio zone.
        float radius = 1.f;                   ///< The radius of the audio zone.

        AudioZoneComponent() = default;

        AudioZoneComponent(const AudioZoneComponent& other) { *this = other; }

        AudioZoneComponent& operator=(const AudioZoneComponent& other)
        {
            if (this != &other)
            {
                zoneID = other.zoneID;
                audioBusName = other.audioBusName;
                position = other.position;
                radius = other.radius;

                AudioZone::CreateZone(*this);
            }
            return *this;
        }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("ZoneID", zoneID), cereal::make_nvp("AudioBusName", audioBusName),
                    cereal::make_nvp("Position", position), cereal::make_nvp("Radius", radius));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("ZoneID", zoneID), cereal::make_nvp("AudioBusName", audioBusName),
                    cereal::make_nvp("Position", position), cereal::make_nvp("Radius", radius));
        }
    };

    struct ScriptComponent
    {
        Ref<Script> script;

        ScriptComponent() = default;
        ScriptComponent(const std::filesystem::path& path, ScriptingLanguage language)
        {
            switch (language)
            {
                using enum ScriptingLanguage;
            case Lua:
                script = ScriptManager::CreateScript(path, language);
                break;
            case cSharp:
                break;
            }
        }

        /**
         * @brief Serializes the ScriptComponent.
         *
         * This function serializes the ScriptComponent by storing the script path and language.
         * Note: Currently, this system only supports Lua scripting language.
         *
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            std::filesystem::path relativePath;
            if (Project::GetActive())
            {
                relativePath =
                    std::filesystem::relative(script->GetPath(), Project::GetActive()->GetProjectDirectory());
            }
            else
            {
                relativePath = script->GetPath();
                COFFEE_CORE_ERROR("ScriptComponent::save: Project is not active, script path is not relative to the "
                                  "project directory!");
            }
            archive(cereal::make_nvp("ScriptPath", relativePath.generic_string()),
                    cereal::make_nvp("Language", ScriptingLanguage::Lua));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            std::string relativePath;
            ScriptingLanguage language;

            archive(cereal::make_nvp("ScriptPath", relativePath), cereal::make_nvp("Language", language));

            if (!relativePath.empty())
            {
                std::filesystem::path scriptPath;
                if (Project::GetActive())
                {
                    scriptPath = Project::GetActive()->GetProjectDirectory() / relativePath;
                }
                else
                {
                    scriptPath = relativePath;
                    COFFEE_CORE_ERROR("ScriptComponent::load: Project is not active, script path is not relative to "
                                      "the project directory!");
                }

                switch (language)
                {
                    using enum ScriptingLanguage;
                case Lua:
                    script = ScriptManager::CreateScript(scriptPath, language);
                    break;
                case cSharp:
                    // Handle cSharp script loading if needed
                    break;
                }
            }
        }
        /*
                static void OnConstruct(entt::registry& registry, entt::entity entity)
                {
                    auto& scriptComponent = registry.get<ScriptComponent<DerivedScript>>(entity);

                    if(Editor is in runtime)
                    {
                        ScriptManager::ExecuteScript(scriptComponent.script);
                        script.OnScenetreeEntered();
                    }
                } */
    };

    struct RigidbodyComponent
    {
      public:
        Ref<RigidBody> rb;
        CollisionCallback callback;

        RigidbodyComponent() = default;
        RigidbodyComponent(const RigidbodyComponent&) = default;
        RigidbodyComponent(const RigidBody::Properties& props, Ref<Collider> collider)
        {
            rb = RigidBody::Create(props, collider);
        }

        ~RigidbodyComponent() { rb.reset(); }

      private:
        friend class cereal::access;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            if (rb)
            {
                archive(cereal::make_nvp("RigidBody", true), cereal::make_nvp("RigidBodyData", rb));
            }
            else
            {
                archive(cereal::make_nvp("RigidBody", false));
            }
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            bool hasRigidBody;
            archive(cereal::make_nvp("RigidBody", hasRigidBody));

            if (hasRigidBody)
            {
                archive(cereal::make_nvp("RigidBodyData", rb));
            }
        }
    };

    struct ParticlesSystemComponent
    {
      public:
        bool NeedsUpdate = true; ///< Flag to indicate if the animator needs an update.
        ParticlesSystemComponent() { m_Particles = CreateRef<ParticleEmitter>(); }

        Ref<ParticleEmitter> GetParticleEmitter() { return m_Particles; }
        void SetParticleEmitter(Ref<ParticleEmitter> newParticleEmitter) { m_Particles = newParticleEmitter; }

        void Emit(int quantity) { m_Particles->Emit(quantity); }
        void SetLooping(bool active) { m_Particles->looping = active; }

      private:
        Ref<ParticleEmitter> m_Particles = nullptr;

      public:
        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("ParticleEmitter", m_Particles));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("ParticleEmitter", m_Particles));
        }


    };

    struct NavMeshComponent
    {
        bool ShowDebug = false; ///< Flag to show the navigation mesh debug.

        /**
         * @brief Gets the navigation mesh.
         * @return The navigation mesh.
         */
        Ref<NavMesh> GetNavMesh() const { return m_NavMesh; }

        /**
         * @brief Sets the navigation mesh.
         * @param navMesh The navigation mesh to set.
         */
        void SetNavMesh(const Ref<NavMesh>& navMesh) { m_NavMesh = navMesh; }

        /**
         * @brief Gets the UUID of the navigation mesh.
         * @return The UUID of the navigation mesh.
         */
        UUID GetNavMeshUUID() const { return m_NavMeshUUID; }

        /**
         * @brief Sets the UUID of the navigation mesh.
         * @param navMeshUUID The UUID of the navigation mesh to set.
         */
        void SetNavMeshUUID(const UUID& navMeshUUID) { m_NavMeshUUID = navMeshUUID; }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("NavMesh", m_NavMesh), cereal::make_nvp("NavMeshUUID", m_NavMeshUUID));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("NavMesh", m_NavMesh), cereal::make_nvp("NavMeshUUID", m_NavMeshUUID));
        }

      private:
        Ref<NavMesh> m_NavMesh = nullptr; ///< The navigation mesh.
        UUID m_NavMeshUUID;               ///< The UUID of the navigation mesh.
    };

    struct NavigationAgentComponent
    {
        std::vector<glm::vec3> Path; ///< The path to follow.
        bool ShowDebug = false;      ///< Flag to show the navigation agent debug.

        /**
         * @brief Finds a path from the start to the end.
         * @param start The start position.
         * @param end The end position.
         * @return The path.
         */
        std::vector<glm::vec3> FindPath(const glm::vec3 start, const glm::vec3 end) const
        {
            return m_PathFinder->FindPath(start, end);
        }

        /**
         * @brief Gets the pathfinder.
         * @return The pathfinder.
         */
        Ref<NavMeshPathfinding> GetPathFinder() const { return m_PathFinder; }

        /**
         * @brief Sets the pathfinder.
         * @param pathFinder The pathfinder to set.
         */
        void SetPathFinder(const Ref<NavMeshPathfinding>& pathFinder) { m_PathFinder = pathFinder; }

        /**
         * @brief Gets the navigation mesh component.
         * @return The navigation mesh component.
         */
        Ref<NavMeshComponent> GetNavMeshComponent() const { return m_NavMeshComponent; }

        /**
         * @brief Sets the navigation mesh component.
         * @param navMeshComponent The navigation mesh component to set.
         */
        void SetNavMeshComponent(const Ref<NavMeshComponent>& navMeshComponent)
        {
            m_NavMeshComponent = navMeshComponent;
        }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("NavMeshComponent", m_NavMeshComponent));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("NavMeshComponent", m_NavMeshComponent));

            m_PathFinder = CreateRef<NavMeshPathfinding>(m_NavMeshComponent->GetNavMesh());
        }

      private:
        Ref<NavMeshPathfinding> m_PathFinder = nullptr;     ///< The pathfinder.
        Ref<NavMeshComponent> m_NavMeshComponent = nullptr; ///< The navigation mesh component.
    };

    struct SpriteComponent
    {
        Ref<Texture2D> texture; ///< The zone ID.
        glm::vec4 tintColor = glm::vec4(1);
        bool flipX = false;
        bool flipY = false;
        float tilingFactor = 1;

        SpriteComponent() { texture = Texture2D::Load("assets/textures/UVMap-Grid.jpg"); };

        SpriteComponent(const SpriteComponent& other) { *this = other; }

        void SetTintColor(glm::vec4 newTint) { tintColor = newTint; }
        glm::vec4 GetTintColor() { return tintColor; }

        SpriteComponent& operator=(const SpriteComponent& other)
        {
            if (this != &other)
            {
                texture = other.texture;
                tintColor = other.tintColor;
                flipX = other.flipX;
                flipY = other.flipY;
                tilingFactor = other.tilingFactor;
            }
            return *this;
        }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("TextureUUID", texture->GetUUID()));
            archive(cereal::make_nvp("TintColor", tintColor));
            archive(cereal::make_nvp("FlipX", flipX));
            archive(cereal::make_nvp("FlipY", flipY));
            archive(cereal::make_nvp("TilingFactor", tilingFactor));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID textureUUID;

            archive(cereal::make_nvp("TextureUUID", textureUUID));
            archive(cereal::make_nvp("TintColor", tintColor));
            archive(cereal::make_nvp("FlipX", flipX));
            archive(cereal::make_nvp("FlipY", flipY));
            archive(cereal::make_nvp("TilingFactor", tilingFactor));

            if (textureUUID)
            {
                texture = ResourceLoader::GetResource<Texture2D>(textureUUID);
            }
        }
    };

    struct ActiveComponent
    {
        ActiveComponent() = default;
        ActiveComponent(const ActiveComponent&) = default;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const {}

        template <class Archive> void load(Archive& archive, std::uint32_t const version) {}
    };

    struct StaticComponent
    {
        StaticComponent() = default;
        StaticComponent(const StaticComponent&) = default;

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const {}

        template <class Archive> void load(Archive& archive, std::uint32_t const version) {}
    };

    struct UIComponent
    {
        RectAnchor Anchor; ///< The anchor of the UI component.
        int Layer = 0;     ///< The layer of the UI component.

        UIComponent() { UIManager::MarkForSorting(); }
        ~UIComponent() { UIManager::MarkForSorting(); }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("Anchor", Anchor), cereal::make_nvp("Layer", Layer));
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("Anchor", Anchor), cereal::make_nvp("Layer", Layer));

            UIManager::MarkForSorting();
        }
    };

    struct UIImageComponent : public UIComponent
    {
        Ref<Texture2D> Texture;                      ///< The texture of the image.
        glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};  ///< The color.
        glm::vec4 UVRect = {0.0f, 0.0f, 1.0f, 1.0f}; ///< The UV rectangle.

        UIImageComponent() { Texture = Texture2D::Load("assets/textures/UVMap-Grid.jpg"); }

        void SetTexture(const Ref<Texture2D>& texture) { Texture = texture; }

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("TextureUUID", Texture ? Texture->GetUUID() : UUID(0)),
                    cereal::make_nvp("Color", Color), cereal::make_nvp("UVRect", UVRect));
            UIComponent::save(archive, version);
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID textureUUID;
            archive(cereal::make_nvp("TextureUUID", textureUUID));
            if (version >= 1)
            {
                archive(cereal::make_nvp("Color", Color), cereal::make_nvp("UVRect", UVRect));
            }
            if (textureUUID != UUID(0))
                Texture = ResourceLoader::GetResource<Texture2D>(textureUUID);
            UIComponent::load(archive, version);
        }
    };

    struct UITextComponent : public UIComponent
    {
        UITextComponent() { Text = "Text"; }

        std::string Text;                                                      ///< The text.
        Ref<Font> UIFont;                                                      ///< The font.
        std::filesystem::path FontPath;                                        ///< The font path.
        glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};                            ///< The color.
        float Kerning = 0.0f;                                                  ///< The kerning.
        float LineSpacing = 0.0f;                                              ///< The line spacing.
        float FontSize = 16.0f;                                                ///< The font size.
        Renderer2D::TextAlignment Alignment = Renderer2D::TextAlignment::Left; ///< The text alignment.

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(
                cereal::make_nvp("Text", Text),
                cereal::make_nvp(
                    "FontPath",
                    std::filesystem::relative(FontPath, Project::GetActive()->GetProjectDirectory()).generic_string()),
                cereal::make_nvp("Color", Color), cereal::make_nvp("Kerning", Kerning),
                cereal::make_nvp("LineSpacing", LineSpacing), cereal::make_nvp("FontSize", FontSize),
                cereal::make_nvp("Alignment", Alignment));
            UIComponent::save(archive, version);
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            std::string relativePath;
            archive(cereal::make_nvp("Text", Text), cereal::make_nvp("FontPath", relativePath),
                    cereal::make_nvp("Color", Color), cereal::make_nvp("Kerning", Kerning),
                    cereal::make_nvp("LineSpacing", LineSpacing), cereal::make_nvp("FontSize", FontSize),
                    cereal::make_nvp("Alignment", Alignment));

            if (!relativePath.empty())
            {
                FontPath = Project::GetActive()->GetProjectDirectory() / relativePath;
                UIFont = CreateRef<Coffee::Font>(FontPath);
            }
            else
                UIFont = Font::GetDefault();
            UIComponent::load(archive, version);
        }
    };

    struct UIToggleComponent : public UIComponent
    {
        UIToggleComponent()
        {
            OnTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            OffTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
        }

        bool Value = false;        ///< The value of the toggle.
        Ref<Texture2D> OnTexture;  ///< The texture when the toggle is on.
        Ref<Texture2D> OffTexture; ///< The texture when the toggle is off.

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("Value", Value),
                    cereal::make_nvp("OnTextureUUID", OnTexture ? OnTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("OffTextureUUID", OffTexture ? OffTexture->GetUUID() : UUID(0)));
            UIComponent::save(archive, version);
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID onTextureUUID;
            UUID offTextureUUID;
            archive(cereal::make_nvp("Value", Value), cereal::make_nvp("OnTextureUUID", onTextureUUID),
                    cereal::make_nvp("OffTextureUUID", offTextureUUID));
            if (onTextureUUID != UUID(0))
                OnTexture = ResourceLoader::GetResource<Texture2D>(onTextureUUID);
            if (offTextureUUID != UUID(0))
                OffTexture = ResourceLoader::GetResource<Texture2D>(offTextureUUID);
            UIComponent::load(archive, version);
        }
    };

    struct UIButtonComponent : public UIComponent
    {
        UIButtonComponent()
        {
            NormalTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            HoverTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            PressedTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            DisabledTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
        }

        enum class State
        {
            Normal,
            Hover,
            Pressed,
            Disabled
        };

        State CurrentState = State::Normal; ///< The current state of the button.
        bool Interactable = true;           ///< Flag to indicate if the button is interactable.

        Ref<Texture2D> NormalTexture;   ///< The texture when the button is normal.
        Ref<Texture2D> HoverTexture;    ///< The texture when the button is hovered.
        Ref<Texture2D> PressedTexture;  ///< The texture when the button is pressed.
        Ref<Texture2D> DisabledTexture; ///< The texture when the button is disabled.

        glm::vec4 NormalColor{1.0f};   ///< The color when the button is normal.
        glm::vec4 HoverColor{1.0f};    ///< The color when the button is hovered.
        glm::vec4 PressedColor{1.0f};  ///< The color when the button is pressed.
        glm::vec4 DisabledColor{1.0f}; ///< The color when the button is disabled.

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("Interactable", Interactable),
                    cereal::make_nvp("NormalTextureUUID", NormalTexture ? NormalTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("HoverTextureUUID", HoverTexture ? HoverTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("PressedTextureUUID", PressedTexture ? PressedTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("DisabledTextureUUID", DisabledTexture ? DisabledTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("NormalColor", NormalColor), cereal::make_nvp("HoverColor", HoverColor),
                    cereal::make_nvp("PressedColor", PressedColor), cereal::make_nvp("DisabledColor", DisabledColor));
            UIComponent::save(archive, version);
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID NormalTextureUUID;
            UUID HoverTextureUUID;
            UUID PressedTextureUUID;
            UUID DisabledTextureUUID;

            archive(cereal::make_nvp("Interactable", Interactable),
                    cereal::make_nvp("NormalTextureUUID", NormalTextureUUID),
                    cereal::make_nvp("HoverTextureUUID", HoverTextureUUID),
                    cereal::make_nvp("PressedTextureUUID", PressedTextureUUID),
                    cereal::make_nvp("DisabledTextureUUID", DisabledTextureUUID),
                    cereal::make_nvp("NormalColor", NormalColor), cereal::make_nvp("HoverColor", HoverColor),
                    cereal::make_nvp("PressedColor", PressedColor), cereal::make_nvp("DisabledColor", DisabledColor));
            if (NormalTextureUUID != UUID(0))
                NormalTexture = ResourceLoader::GetResource<Texture2D>(NormalTextureUUID);
            if (HoverTextureUUID != UUID(0))
                HoverTexture = ResourceLoader::GetResource<Texture2D>(HoverTextureUUID);
            if (PressedTextureUUID != UUID(0))
                PressedTexture = ResourceLoader::GetResource<Texture2D>(PressedTextureUUID);
            if (DisabledTextureUUID != UUID(0))
                DisabledTexture = ResourceLoader::GetResource<Texture2D>(DisabledTextureUUID);
            UIComponent::load(archive, version);
        }
    };

    struct UISliderComponent : public UIComponent
    {
        UISliderComponent()
        {
            BackgroundTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            HandleTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            DisabledHandleTexture = Texture2D::Load("assets/textures/UVMap-Grid.jpg");
            HandleScale = {1.0f, 1.0f};
        }

        float Value = 0.0f;                   ///< The value of the slider.
        float MinValue = 0.0f;                ///< The minimum value of the slider.
        float MaxValue = 100.0f;              ///< The maximum value of the slider.
        glm::vec2 HandleScale;                ///< The scale of the handle.
        Ref<Texture2D> BackgroundTexture;     ///< The texture of the background.
        Ref<Texture2D> HandleTexture;         ///< The texture of the handle.
        Ref<Texture2D> DisabledHandleTexture; ///< The texture of the disabled handle.
        bool Selected = false;                ///< Flag to indicate if the slider is selected.

        template <class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(
                cereal::make_nvp("Value", Value), cereal::make_nvp("MinValue", MinValue),
                cereal::make_nvp("MaxValue", MaxValue), cereal::make_nvp("HandleScale", HandleScale),
                cereal::make_nvp("BackgroundTextureUUID", BackgroundTexture ? BackgroundTexture->GetUUID() : UUID(0)),
                cereal::make_nvp("HandleTextureUUID", HandleTexture ? HandleTexture->GetUUID() : UUID(0)));
            if (version >= 1)
            {
                archive(cereal::make_nvp("DisabledHandleTextureUUID",
                                         DisabledHandleTexture ? DisabledHandleTexture->GetUUID() : UUID(0)));
            }
            UIComponent::save(archive, version);
        }

        template <class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            UUID BackgroundTextureUUID;
            UUID HandleTextureUUID;
            UUID DisabledHandleTextureUUID;

            archive(cereal::make_nvp("Value", Value), cereal::make_nvp("MinValue", MinValue),
                    cereal::make_nvp("MaxValue", MaxValue), cereal::make_nvp("HandleScale", HandleScale),
                    cereal::make_nvp("BackgroundTextureUUID", BackgroundTextureUUID),
                    cereal::make_nvp("HandleTextureUUID", HandleTextureUUID));

            if (BackgroundTextureUUID != UUID(0))
                BackgroundTexture = ResourceLoader::GetResource<Texture2D>(BackgroundTextureUUID);
            if (HandleTextureUUID != UUID(0))
                HandleTexture = ResourceLoader::GetResource<Texture2D>(HandleTextureUUID);

            if (version >= 1)
            {
                archive(cereal::make_nvp("DisabledHandleTextureUUID", DisabledHandleTextureUUID));

                if (DisabledHandleTextureUUID != UUID(0))
                    DisabledHandleTexture = ResourceLoader::GetResource<Texture2D>(DisabledHandleTextureUUID);
            }
            UIComponent::load(archive, version);
        }
    };

} // namespace Coffee
CEREAL_CLASS_VERSION(Coffee::TagComponent, 0);
CEREAL_CLASS_VERSION(Coffee::TransformComponent, 0);
CEREAL_CLASS_VERSION(Coffee::CameraComponent, 0);
CEREAL_CLASS_VERSION(Coffee::AnimatorComponent, 0);
CEREAL_CLASS_VERSION(Coffee::MeshComponent, 0);
CEREAL_CLASS_VERSION(Coffee::MaterialComponent, 1);
CEREAL_CLASS_VERSION(Coffee::LightComponent, 1);
CEREAL_CLASS_VERSION(Coffee::WorldEnvironmentComponent, 1);
CEREAL_CLASS_VERSION(Coffee::AudioSourceComponent, 1);
CEREAL_CLASS_VERSION(Coffee::AudioListenerComponent, 1);
CEREAL_CLASS_VERSION(Coffee::AudioZoneComponent, 0);
CEREAL_CLASS_VERSION(Coffee::ScriptComponent, 0);
CEREAL_CLASS_VERSION(Coffee::RigidbodyComponent, 0);
CEREAL_CLASS_VERSION(Coffee::NavMeshComponent, 0);
CEREAL_CLASS_VERSION(Coffee::NavigationAgentComponent, 0);
CEREAL_CLASS_VERSION(Coffee::ParticlesSystemComponent, 0);
CEREAL_CLASS_VERSION(Coffee::UIComponent, 0);
CEREAL_CLASS_VERSION(Coffee::UIImageComponent, 1);
CEREAL_CLASS_VERSION(Coffee::UITextComponent, 0);
CEREAL_CLASS_VERSION(Coffee::UIToggleComponent, 0);
CEREAL_CLASS_VERSION(Coffee::UIButtonComponent, 0);
CEREAL_CLASS_VERSION(Coffee::UISliderComponent, 1);

/** @} */
