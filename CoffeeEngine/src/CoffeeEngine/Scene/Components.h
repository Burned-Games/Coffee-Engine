/**
 * @defgroup scene Scene
 * @{
 */

 #pragma once

 #include "CoffeeEngine/Core/Base.h"
 #include "CoffeeEngine/IO/ResourceLoader.h"
 #include "CoffeeEngine/IO/ResourceRegistry.h"
 #include "CoffeeEngine/Physics/Collider.h"
 #include "CoffeeEngine/Animation/AnimationSystem.h"
 #include "CoffeeEngine/Physics/RigidBody.h"
 #include "CoffeeEngine/Renderer/Material.h"
 #include "CoffeeEngine/Renderer/Mesh.h"
 #include "CoffeeEngine/Scene/SceneCamera.h"
 #include "CoffeeEngine/Scripting/Script.h"
 #include "CoffeeEngine/Scripting/ScriptManager.h"
 #include "CoffeeEngine/Renderer/Texture.h"
 #include "CoffeeEngine/Renderer/Font.h"
 #include "CoffeeEngine/ParticleManager/ParticleManager.h"
 #include "CoffeeEngine/Navigation/NavMesh.h"
 #include "CoffeeEngine/Navigation/NavMeshPathfinding.h"

 #include <cereal/cereal.hpp>
 #include <cereal/access.hpp>
 #include <cereal/cereal.hpp>
 #include <cereal/types/memory.hpp>
 #include <cereal/types/vector.hpp>
 #include <cereal/types/string.hpp>
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

 
 
 namespace Coffee {
     /**
      * @brief Component representing a tag.
      * @ingroup scene
      */
     struct TagComponent
     {
         std::string Tag; ///< The tag string.
 
         TagComponent() = default;
         TagComponent(const TagComponent&) = default;
         TagComponent(const std::string& tag)
             : Tag(tag) {}
 
         /**
          * @brief Serializes the TagComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to serialize to.
          */
         template<class Archive>
         void serialize(Archive& archive)
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
         glm::mat4 worldMatrix = glm::mat4(1.0f); ///< The world transformation matrix.
     public:
         glm::vec3 Position = { 0.0f, 0.0f, 0.0f }; ///< The position vector.
         glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f }; ///< The rotation vector.
         glm::vec3 Scale = { 1.0f, 1.0f, 1.0f }; ///< The scale vector.
 
         TransformComponent() = default;
         TransformComponent(const TransformComponent&) = default;
         TransformComponent(const glm::vec3& position)
             : Position(position) {}
 
         /**
          * @brief Gets the local transformation matrix.
          * @return The local transformation matrix.
          */
         glm::mat4 GetLocalTransform() const
         {
             glm::mat4 rotation = glm::toMat4(glm::quat(glm::radians(Rotation)));
 
             return glm::translate(glm::mat4(1.0f), Position)
                     * rotation
                     * glm::scale(glm::mat4(1.0f), Scale);
         }
 
         /**
          * @brief Sets the local transformation matrix.
          * @param transform The transformation matrix to set.
          */
         void SetLocalTransform(const glm::mat4& transform) //TODO: Improve this function, this way is ugly and glm::decompose is from gtx (is supposed to not be very stable)
         {
             glm::vec3 skew;
             glm::vec4 perspective;
             glm::quat orientation;
 
             glm::decompose(transform, Scale, orientation, Position, skew, perspective);
             Rotation = glm::degrees(glm::eulerAngles(orientation));
         }
 
         /**
          * @brief Gets the world transformation matrix.
          * @return The world transformation matrix.
          */
         const glm::mat4& GetWorldTransform() const
         {
             return worldMatrix;
         }
 
         /**
          * @brief Sets the world transformation matrix.
          * @param transform The transformation matrix to set.
          */
         void SetWorldTransform(const glm::mat4& transform)
         {
             worldMatrix = transform * GetLocalTransform();
         }
 
         /**
          * @brief Serializes the TransformComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to serialize to.
          */
         template<class Archive>
         void serialize(Archive& archive)
         {
             archive(cereal::make_nvp("Position", Position), cereal::make_nvp("Rotation", Rotation), cereal::make_nvp("Scale", Scale));
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
         template<class Archive>
         void serialize(Archive& archive)
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
         : IsBlending(other.IsBlending),
           Loop(other.Loop),
           CurrentAnimation(other.CurrentAnimation),
           NextAnimation(other.NextAnimation),
           AnimationTime(other.AnimationTime),
           NextAnimationTime(other.NextAnimationTime),
           BlendTime(other.BlendTime),
           BlendDuration(other.BlendDuration),
           BlendThreshold(other.BlendThreshold),
           AnimationSpeed(other.AnimationSpeed),
           JointMatrices(other.JointMatrices),
           modelUUID(other.modelUUID),
           animatorUUID(other.animatorUUID),
           m_Skeleton(other.m_Skeleton),
           m_AnimationController(other.m_AnimationController)
         {
             m_BlendJob.layers = ozz::make_span(m_BlendLayers);
             AnimationSystem::SetCurrentAnimation(CurrentAnimation, this);
             AnimationSystem::AddAnimator(this);
         }
 
         /**
          * @brief Constructs an AnimatorComponent with the given skeleton, animation controller, and animation system.
          * @param skeleton The skeleton reference.
          * @param animationController The animation controller reference.
          */
         AnimatorComponent(Ref<Skeleton> skeleton, Ref<AnimationController> animationController)
         : m_Skeleton(std::move(skeleton)), m_AnimationController(std::move(animationController))
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
         void SetSkeleton(Ref<Skeleton> skeleton) { m_Skeleton = skeleton; }
 
         /**
          * @brief Gets the animation controller reference.
          * @return The animation controller reference.
          */
         Ref<AnimationController> GetAnimationController() const { return m_AnimationController; }

         /**
          * @brief Sets the animation controller reference.
          * @param animationController The animation controller reference to set.
          */
         void SetAnimationController(Ref<AnimationController> animationController) { m_AnimationController = animationController; }

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
 
 
         void SetCurrentAnimation(int index) { AnimationSystem::SetCurrentAnimation(index, this);}
 
         /**
          * @brief Serializes the AnimatorComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to serialize to.
          */
         template<class Archive>
         void save(Archive& archive) const
         {
             archive(cereal::make_nvp("CurrentAnimation", CurrentAnimation),
                     cereal::make_nvp("BlendDuration", BlendDuration),
                     cereal::make_nvp("BlendThreshold", BlendThreshold),
                     cereal::make_nvp("AnimationSpeed", AnimationSpeed),
                     cereal::make_nvp("Loop", Loop),
                     cereal::make_nvp("ModelUUID", modelUUID),
                     cereal::make_nvp("AnimatorUUID", animatorUUID));
         }
 
         /**
          * @brief Deserializes the AnimatorComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to deserialize from.
          */
         template<class Archive>
         void load(Archive& archive)
         {
             archive(cereal::make_nvp("CurrentAnimation", CurrentAnimation),
                     cereal::make_nvp("BlendDuration", BlendDuration),
                     cereal::make_nvp("BlendThreshold", BlendThreshold),
                     cereal::make_nvp("AnimationSpeed", AnimationSpeed),
                     cereal::make_nvp("Loop", Loop),
                     cereal::make_nvp("ModelUUID", modelUUID),
                     cereal::make_nvp("AnimatorUUID", animatorUUID));

             AnimationSystem::LoadAnimator(this);
         }
 
     public:
         bool IsBlending = false; ///< Indicates if the animation is blending.
         bool Loop = true; ///< Indicates if the animation should loop.
         unsigned int CurrentAnimation = 0; ///< The current animation index.
         unsigned int NextAnimation = 0; ///< The next animation index.
         float AnimationTime = 0.f; ///< The current animation time.
         float NextAnimationTime = 0.f; ///< The next animation time.
         float BlendTime = 0.f; ///< The current blend time.
         float BlendDuration = 0.25f; ///< The duration of the blend.
         float BlendThreshold = 0.8; ///< The blend threshold.
         float AnimationSpeed = 1.0f; ///< The speed of the animation.
 
         std::vector<glm::mat4> JointMatrices; ///< The joint matrices.
         UUID modelUUID; ///< The UUID of the model.
         UUID animatorUUID; ///< The UUID of the animator.

     private:
         Ref<Skeleton> m_Skeleton; ///< The skeleton reference.
         Ref<AnimationController> m_AnimationController; ///< The animation controller reference.
 
         ozz::animation::SamplingJob::Context m_Context; ///< The sampling job context.
         ozz::animation::BlendingJob::Layer m_BlendLayers[2]; ///< The blend layers.
         ozz::animation::BlendingJob m_BlendJob; ///< The blending job.
     };
 
     /**
      * @brief Component representing a mesh.
      * @ingroup scene
      */
     struct MeshComponent
     {
         Ref<Mesh> mesh; ///< The mesh reference.
         bool drawAABB = false; ///< Flag to draw the axis-aligned bounding box (AABB).
 
         AnimatorComponent* animator = nullptr; ///< The animator component.
         UUID animatorUUID = 0; ///< The UUID of the animator.
 
         MeshComponent()
         {
         }
         MeshComponent(const MeshComponent&) = default;
         MeshComponent(Ref<Mesh> mesh)
             : mesh(mesh) {}
 
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
         template<class Archive>
         void save(Archive& archive) const
         {
             archive(cereal::make_nvp("Mesh", mesh->GetUUID()),
                     cereal::make_nvp("AnimatorUUID", animatorUUID));
 
             if (animator && animatorUUID != 0)
                 animator->animatorUUID = animatorUUID;
         }
 
         template<class Archive>
         void load(Archive& archive)
         {
             UUID meshUUID;
             archive(cereal::make_nvp("Mesh", meshUUID),
                     cereal::make_nvp("AnimatorUUID", animatorUUID));
 
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
 
         MaterialComponent()
         {
         }
         MaterialComponent(const MaterialComponent&) = default;
         MaterialComponent(Ref<Material> material)
             : material(material) {}
 
         private:
             friend class cereal::access;
         /**
          * @brief Serializes the MeshComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to serialize to.
          */
         template<class Archive>
         void save(Archive& archive) const
         {
            ResourceSaver::SaveToCache<Material>(material->GetUUID(), material);
            archive(cereal::make_nvp("Material", material->GetUUID()));
         }
 
         template<class Archive>
         void load(Archive& archive)
         {
             UUID materialUUID;
             archive(cereal::make_nvp("Material", materialUUID));
 
             Ref<Material> material = ResourceLoader::GetResource<Material>(materialUUID);
             this->material = material;
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
             PointLight = 1, ///< Point light.
             SpotLight = 2 ///< Spot light.
         };
 
         // Align to 16 bytes(glm::vec4) instead of 12 bytes(glm::vec3) to match the std140 layout in the shader (a vec3 is 16 bytes in std140)
         alignas(16) glm::vec3 Color = {1.0f, 1.0f, 1.0f}; ///< The color of the light.
         alignas(16) glm::vec3 Direction = {0.0f, -1.0f, 0.0f}; ///< The direction of the light.
         alignas(16) glm::vec3 Position = {0.0f, 0.0f, 0.0f}; ///< The position of the light.
 
         float Range = 5.0f; ///< The range of the light.
         float Attenuation = 1.0f; ///< The attenuation of the light.
         float Intensity = 1.0f; ///< The intensity of the light.
 
         float Angle = 45.0f; ///< The angle of the light.
 
         int type = static_cast<int>(Type::DirectionalLight); ///< The type of the light.
 
         LightComponent() = default;
         LightComponent(const LightComponent&) = default;
 
         /**
          * @brief Serializes the LightComponent.
          * @tparam Archive The type of the archive.
          * @param archive The archive to serialize to.
          */
         template<class Archive>
         void serialize(Archive& archive)
         {
             archive(cereal::make_nvp("Color", Color), cereal::make_nvp("Direction", Direction), cereal::make_nvp("Position", Position), cereal::make_nvp("Range", Range), cereal::make_nvp("Attenuation", Attenuation), cereal::make_nvp("Intensity", Intensity), cereal::make_nvp("Angle", Angle), cereal::make_nvp("Type", type));
         }
     };
 
     struct AudioSourceComponent
     {
         uint64_t gameObjectID = -1; ///< The object ID.
         Ref<Audio::AudioBank> audioBank; ///< The audio bank.
         std::string audioBankName; ///< The name of the audio bank.
         std::string eventName; ///< The name of the event.
         float volume = 1.f; ///< The volume of the audio source.
         bool mute = false; ///< True if the audio source is muted.
         bool playOnAwake = false; ///< True if the audio source should play automatically.
         glm::mat4 transform; ///< The transform of the audio source.
         bool isPlaying = false; ///< True if the audio source is playing.
         bool isPaused = false; ///< True if the audio source is paused.
         bool toDelete = false; ///< True if the audio source should be deleted.
 
         AudioSourceComponent() = default;
 
         AudioSourceComponent(const AudioSourceComponent& other)
         {
             *this = other;
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
 
                 if (!toDelete)
                 {
                     Audio::RegisterAudioSourceComponent(*this);
                     AudioZone::RegisterObject(gameObjectID, transform[3]);
                 }
             }
             return *this;
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
 
 
         template<class Archive>
         void save(Archive& archive) const
         {
             archive(cereal::make_nvp("GameObjectID", gameObjectID),
                     cereal::make_nvp("AudioBank", audioBank),
                     cereal::make_nvp("AudioBankName", audioBankName),
                     cereal::make_nvp("EventName", eventName),
                     cereal::make_nvp("Volume", volume),
                     cereal::make_nvp("Mute", mute),
                     cereal::make_nvp("PlayOnAwake", playOnAwake),
                     cereal::make_nvp("Transform", transform)
             );
         }
 
         template<class Archive>
         void load(Archive& archive)
         {
             archive(cereal::make_nvp("GameObjectID", gameObjectID),
                     cereal::make_nvp("AudioBank", audioBank),
                     cereal::make_nvp("AudioBankName", audioBankName),
                     cereal::make_nvp("EventName", eventName),
                     cereal::make_nvp("Volume", volume),
                     cereal::make_nvp("Mute", mute),
                     cereal::make_nvp("PlayOnAwake", playOnAwake),
                     cereal::make_nvp("Transform", transform)
             );
         }
     };
 
     struct AudioListenerComponent
     {
         uint64_t gameObjectID = -1; ///< The object ID.
         glm::mat4 transform; ///< The transform of the audio listener.
         bool toDelete = false; ///< True if the audio listener should be deleted.
 
         AudioListenerComponent() = default;
 
         AudioListenerComponent(const AudioListenerComponent& other)
         {
             *this = other;
         }
 
         AudioListenerComponent& operator=(const AudioListenerComponent& other)
         {
             if (this != &other)
             {
                 gameObjectID = other.gameObjectID;
                 transform = other.transform;
                 toDelete = other.toDelete;
 
                 if (!toDelete)
                     Audio::RegisterAudioListenerComponent(*this);
             }
             return *this;
         }
 
         template<class Archive>
         void save(Archive& archive) const
         {
             archive(cereal::make_nvp("GameObjectID", gameObjectID),
                     cereal::make_nvp("Transform", transform)
             );
         }
 
         template<class Archive>
         void load(Archive& archive)
         {
             archive(cereal::make_nvp("GameObjectID", gameObjectID),
                     cereal::make_nvp("Transform", transform)
             );
         }
     };
 
     struct AudioZoneComponent
     {
         uint64_t zoneID = -1; ///< The zone ID.
         std::string audioBusName; ///< The name of the audio bus.
         glm::vec3 position = { 0.f, 0.f, 0.f }; ///< The position of the audio zone.
         float radius = 1.f; ///< The radius of the audio zone.
 
         AudioZoneComponent() = default;
 
         AudioZoneComponent(const AudioZoneComponent& other)
         {
             *this = other;
         }
 
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
 
         template<class Archive>
         void save(Archive& archive) const
         {
             archive(cereal::make_nvp("ZoneID", zoneID),
                     cereal::make_nvp("AudioBusName", audioBusName),
                     cereal::make_nvp("Position", position),
                     cereal::make_nvp("Radius", radius)
             );
         }
 
         template<class Archive>
         void load(Archive& archive)
         {
             archive(cereal::make_nvp("ZoneID", zoneID),
                     cereal::make_nvp("AudioBusName", audioBusName),
                     cereal::make_nvp("Position", position),
                     cereal::make_nvp("Radius", radius)
             );
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
         template<class Archive>
         void save(Archive& archive) const
         {
             std::filesystem::path relativePath;
             if (Project::GetActive())
             {
                 relativePath = std::filesystem::relative(script->GetPath(), Project::GetActive()->GetProjectDirectory());
             }
             else
             {
                 relativePath = script->GetPath();
                 COFFEE_CORE_ERROR("ScriptComponent::save: Project is not active, script path is not relative to the project directory!");
             }
             archive(cereal::make_nvp("ScriptPath", relativePath.string()), cereal::make_nvp("Language", ScriptingLanguage::Lua));
         }
 
         template<class Archive>
         void load(Archive& archive)
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
                     COFFEE_CORE_ERROR("ScriptComponent::load: Project is not active, script path is not relative to the project directory!");
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
 
     struct RigidbodyComponent {
         public:
             Ref<RigidBody> rb;
             CollisionCallback callback;
 
             RigidbodyComponent() = default;
             RigidbodyComponent(const RigidbodyComponent&) = default;
             RigidbodyComponent(const RigidBody::Properties& props, Ref<Collider> collider) {
                 rb = RigidBody::Create(props, collider);
             }
 
             ~RigidbodyComponent() {
                 rb.reset();
             }
 
         private:
             friend class cereal::access;
 
             template<class Archive>
             void save(Archive& archive) const {
                 if (rb) {
                     archive(
                         cereal::make_nvp("RigidBody", true),
                         cereal::make_nvp("RigidBodyData", rb)
                     );
                 } else {
                     archive(cereal::make_nvp("RigidBody", false));
                 }
             }
 
             template<class Archive>
             void load(Archive& archive) {
                 bool hasRigidBody;
                 archive(cereal::make_nvp("RigidBody", hasRigidBody));
                 
                 if (hasRigidBody) {
                     archive(cereal::make_nvp("RigidBodyData", rb));
                 }
             }
     };
  
    struct UICanvasComponent
    {
        UICanvasComponent() = default;
        UICanvasComponent(const UICanvasComponent&) = default;
        Ref<Texture2D> CanvasTexture;
        bool Visible = true;

        template<class Archive>
        void save(Archive& archive) const {
                    cereal::make_nvp("Visible", Visible);
        }

        template<class Archive>
        void load(Archive& archive) {
                    cereal::make_nvp("Visible", Visible);
        }
    };

     struct UIImageComponent
     {
         Ref<Texture2D> texture;
         glm::vec2 Size = {1.0f, 1.0f};
         bool Visible = true;

         UIImageComponent() = default;
         UIImageComponent(const std::string& texturePath, const glm::vec2& size, bool visible)
            : Size(size), Visible(visible) {
             if (!texturePath.empty())
             {
                 texture = Texture2D::Load(texturePath);
             }
         }

         void SetTexture(const Ref<Texture2D>& newTexture) {
             texture = newTexture;
         }

         void SetTexture(const std::string& texturePath) {
             if (!texturePath.empty())
             {
                 texture = Texture2D::Load(texturePath);
             }
         }

         template<class Archive>
         void save(Archive& archive) const
         {
            

            archive(cereal::make_nvp("TextureUUID", texture->GetUUID()),
                     cereal::make_nvp("Size", Size),
                     cereal::make_nvp("Visible", Visible));
         }

         template<class Archive>
         void load(Archive& archive) {

             UUID textureUUID;

             archive(cereal::make_nvp("TextureUUID", textureUUID),
                     cereal::make_nvp("Size", Size),
                     cereal::make_nvp("Visible", Visible));

             if (textureUUID)
             {
                 texture = ResourceLoader::GetResource<Texture2D>(textureUUID);
             }

          
         }
     };

    struct UITextComponent
    {
        std::string Text = "Default Text";
        std::string FontPath;
        Ref<Font> font;
        float FontSize = 24.0f;
        glm::vec4 Color = {1.0f, 1.0f, 1.0f, 1.0f};
        bool Visible = true;

        UITextComponent() = default;
        UITextComponent(const std::string& text, const std::string& fontPath, const glm::vec2 position, float fontSize, float lineSpacing, const glm::vec4 color, bool visible)
            : Text(text), FontPath(fontPath), FontSize(fontSize), Color(color), Visible(visible)
        {
            if (!fontPath.empty())
            {
               font = Font::GetDefault();
            }
        }

        template<class Archive>
        void save(Archive& archive) const {
            archive(cereal::make_nvp("Text", Text),
                    cereal::make_nvp("FontPath", FontPath),
                    cereal::make_nvp("FontSize", FontSize),
                    cereal::make_nvp("Color", Color),
                    cereal::make_nvp("Visible", Visible));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("Text", Text),
                    cereal::make_nvp("FontPath", FontPath),
                    cereal::make_nvp("FontSize", FontSize),
                    cereal::make_nvp("Color", Color),
                    cereal::make_nvp("Visible", Visible));
        }

    };

  struct UISliderComponent
    {
        Ref<Texture2D> barTexture;
        Ref<Texture2D> handleTexture;
        glm::vec2 Size = {300.0f, 50.0f};
        glm::vec2 HandleSize = {75.0f, 75.0f};
        float Value = 0.5f;
        bool Visible = true;

        UISliderComponent() = default;


        UISliderComponent(const std::string& barTexturePath, const std::string& handleTexturePath, const glm::vec2& size, const glm::vec2& handleSize, bool visible)
            : Size(size), HandleSize(handleSize), Visible(visible)
        {
            if (!barTexturePath.empty())
            {
                barTexture = Texture2D::Load(barTexturePath);
            }
            if (!handleTexturePath.empty())
            {
                handleTexture = Texture2D::Load(handleTexturePath);
            }
        }


        void SetBarTexture(const Ref<Texture2D>& newTexture) { barTexture = newTexture; }
        void SetHandleTexture(const Ref<Texture2D>& newTexture) { handleTexture = newTexture; }

        void SetBarTexture(const std::string& texturePath)
        {
            if (!texturePath.empty())
            {
                barTexture = Texture2D::Load(texturePath);
            }
        }

        void SetHandleTexture(const std::string& texturePath)
        {
            if (!texturePath.empty())
            {
                handleTexture = Texture2D::Load(texturePath);
            }
        }


        void SetValue(float newValue)
        {
            Value = glm::clamp(newValue, 0.0f, 1.0f);
        }


        template<class Archive>
        void save(Archive& archive) const
        {
            archive(
                cereal::make_nvp("BarTextureUUID", barTexture->GetUUID()),
                cereal::make_nvp("HandleTextureUUID", handleTexture->GetUUID()),
                cereal::make_nvp("Size", Size),
                cereal::make_nvp("HandleSize", HandleSize),
                cereal::make_nvp("Value", Value),
                cereal::make_nvp("Visible", Visible)
            );
        }


        template<class Archive>
        void load(Archive& archive)
        {
            UUID barTextureUUID, handleTextureUUID;

            archive(
                cereal::make_nvp("BarTextureUUID", barTextureUUID),
                cereal::make_nvp("HandleTextureUUID", handleTextureUUID),
                cereal::make_nvp("Size", Size),
                cereal::make_nvp("HandleSize", HandleSize),
                cereal::make_nvp("Value", Value),
                cereal::make_nvp("Visible", Visible)
            );

            if (barTextureUUID)
            {
                barTexture = ResourceLoader::GetResource<Texture2D>(barTextureUUID);
            }
            if (handleTextureUUID)
            {
                handleTexture = ResourceLoader::GetResource<Texture2D>(handleTextureUUID);
            }
        }
    };

    struct UIButtonComponent
    {
        bool Visible = true;

        Ref<Texture2D> baseTexture;
        Ref<Texture2D> selectedTexture;
        Ref<Texture2D> pressedTexture;

        glm::vec2 baseSize = {100.0f, 100.0f};
        glm::vec2 selectedSize = {120.0f, 120.0f};
        glm::vec2 pressedSize = {90.0f, 90.0f};

        glm::vec4 baseColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 selectedColor = {0.8f, 0.8f, 1.0f, 1.0f};
        glm::vec4 pressedColor = {0.6f, 0.6f, 1.0f, 1.0f};

        enum class ButtonState
        {
            Base,
            Selected,
            Pressed
        };

        ButtonState currentState = ButtonState::Base; // Estado actual del botón

        UIButtonComponent() = default;

        UIButtonComponent(const std::string& baseTexturePath, const std::string& selectedTexturePath, const std::string& pressedTexturePath)
        {
            if (!baseTexturePath.empty())
            {
                baseTexture = Texture2D::Load(baseTexturePath);
            }
            if (!selectedTexturePath.empty())
            {
                selectedTexture = Texture2D::Load(selectedTexturePath);
            }
            if (!pressedTexturePath.empty())
            {
                pressedTexture = Texture2D::Load(pressedTexturePath);
            }
        }

        void SetState(ButtonState newState)
        {
            currentState = newState;
        }

        Ref<Texture2D> GetCurrentTexture() const
        {
            switch (currentState)
            {
            case ButtonState::Selected:
                return selectedTexture;
            case ButtonState::Pressed:
                return pressedTexture;
            default:
                return baseTexture;
            }
        }

        glm::vec2 GetCurrentSize() const
        {
            switch (currentState)
            {
            case ButtonState::Selected:
                return selectedSize;
            case ButtonState::Pressed:
                return pressedSize;
            default:
                return baseSize;
            }
        }

        glm::vec4 GetCurrentColor() const
        {
            switch (currentState)
            {
            case ButtonState::Selected:
                return selectedColor;
            case ButtonState::Pressed:
                return pressedColor;
            default:
                return baseColor;
            }
        }

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(
                cereal::make_nvp("BaseTextureUUID", baseTexture ? baseTexture->GetUUID() : UUID(0)),
                cereal::make_nvp("SelectedTextureUUID", selectedTexture ? selectedTexture->GetUUID() : UUID(0)),
                cereal::make_nvp("PressedTextureUUID", pressedTexture ? pressedTexture->GetUUID() : UUID(0)),
                cereal::make_nvp("BaseSize", baseSize),
                cereal::make_nvp("SelectedSize", selectedSize),
                cereal::make_nvp("PressedSize", pressedSize),
                cereal::make_nvp("BaseColor", baseColor),
                cereal::make_nvp("SelectedColor", selectedColor),
                cereal::make_nvp("PressedColor", pressedColor),
                cereal::make_nvp("Visible", Visible),
                cereal::make_nvp("CurrentState", static_cast<int>(currentState)) // Añadir estado actual
            );
        }

        template<class Archive>
        void load(Archive& archive)
        {
            UUID baseTextureUUID, selectedTextureUUID, pressedTextureUUID;
            int loadedState;

            archive(
                cereal::make_nvp("BaseTextureUUID", baseTextureUUID),
                cereal::make_nvp("SelectedTextureUUID", selectedTextureUUID),
                cereal::make_nvp("PressedTextureUUID", pressedTextureUUID),
                cereal::make_nvp("BaseSize", baseSize),
                cereal::make_nvp("SelectedSize", selectedSize),
                cereal::make_nvp("PressedSize", pressedSize),
                cereal::make_nvp("BaseColor", baseColor),
                cereal::make_nvp("SelectedColor", selectedColor),
                cereal::make_nvp("PressedColor", pressedColor),
                cereal::make_nvp("Visible", Visible),
                cereal::make_nvp("CurrentState", loadedState)
            );

            currentState = static_cast<ButtonState>(loadedState);

            if (baseTextureUUID != 0) {
                baseTexture = ResourceLoader::GetResource<Texture2D>(baseTextureUUID);
            }
            if (selectedTextureUUID != 0) {
                selectedTexture = ResourceLoader::GetResource<Texture2D>(selectedTextureUUID);
            }
            if (pressedTextureUUID != 0) {
                pressedTexture = ResourceLoader::GetResource<Texture2D>(pressedTextureUUID);
            }
        }
    };


    struct UIToggleComponent
    {
        Ref<Texture2D> ActiveTexture = Texture2D::Load("assets/textures/toggleON.png");
        Ref<Texture2D> InactiveTexture = Texture2D::Load("assets/textures/toggleOFF.png");
        glm::vec2 Size = {100.0f, 100.0f};
        bool IsActive = false;
        bool Visible = true;

        UIToggleComponent() = default;

        UIToggleComponent(const std::string& activeTexturePath, const std::string& inactiveTexturePath,
                          const glm::vec2& size, bool visible)
            : Size(size), Visible(visible)
        {
            if (!activeTexturePath.empty())
            {
                ActiveTexture = Texture2D::Load(activeTexturePath);
            }
            if (!inactiveTexturePath.empty())
            {
                InactiveTexture = Texture2D::Load(inactiveTexturePath);
            }
        }

        void SetActiveTexture(const Ref<Texture2D>& newTexture) { ActiveTexture = newTexture; }
        void SetInactiveTexture(const Ref<Texture2D>& newTexture) { InactiveTexture = newTexture; }

        void SetActiveTexture(const std::string& texturePath)
        {
            if (!texturePath.empty())
            {
                ActiveTexture = Texture2D::Load(texturePath);
            }
        }

        void SetInactiveTexture(const std::string& texturePath)
        {
            if (!texturePath.empty())
            {
                InactiveTexture = Texture2D::Load(texturePath);
            }
        }

        void Toggle() { IsActive = !IsActive; }

        template <class Archive> void save(Archive& archive) const
        {
            archive(cereal::make_nvp("ActiveTextureUUID", ActiveTexture ? ActiveTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("InactiveTextureUUID", InactiveTexture ? InactiveTexture->GetUUID() : UUID(0)),
                    cereal::make_nvp("Size", Size), cereal::make_nvp("IsActive", IsActive),
                    cereal::make_nvp("Visible", Visible));
        }

        template <class Archive> void load(Archive& archive)
        {
            UUID activeTextureUUID, inactiveTextureUUID;

            archive(cereal::make_nvp("ActiveTextureUUID", activeTextureUUID),
                    cereal::make_nvp("InactiveTextureUUID", inactiveTextureUUID), cereal::make_nvp("Size", Size),
                    cereal::make_nvp("IsActive", IsActive), cereal::make_nvp("Visible", Visible));

            if (activeTextureUUID)
            {
                ActiveTexture = ResourceLoader::GetResource<Texture2D>(activeTextureUUID);
            }
            if (inactiveTextureUUID)
            {
                InactiveTexture = ResourceLoader::GetResource<Texture2D>(inactiveTextureUUID);
            }
        }
    };

    struct ParticlesSystemComponent
    {
        public:
        // Constructor por defecto
        ParticlesSystemComponent() { 
            m_Particles = CreateRef<ParticleEmitter>();
            
        }
        

        Ref<ParticleEmitter> GetParticleEmitter() { return m_Particles; }

        void Emit(int quantity) { m_Particles->Emit(quantity); }
        void SetLooping(bool active) { m_Particles->looping = active; }


        private:
        Ref<ParticleEmitter> m_Particles = nullptr; 


        public:
        template <class Archive> void save(Archive& archive) const
        {
            archive(cereal::make_nvp("ParticleEmitter", m_Particles));
        }

        template <class Archive> void load(Archive& archive)
        {
            archive(cereal::make_nvp("ParticleEmitter", m_Particles) );
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

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("NavMesh", m_NavMesh), cereal::make_nvp("NavMeshUUID", m_NavMeshUUID));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("NavMesh", m_NavMesh), cereal::make_nvp("NavMeshUUID", m_NavMeshUUID));
        }

    private:
        Ref<NavMesh> m_NavMesh = nullptr; ///< The navigation mesh.
        UUID m_NavMeshUUID; ///< The UUID of the navigation mesh.
    };

    struct NavigationAgentComponent
    {
        std::vector<glm::vec3> Path; ///< The path to follow.
        bool ShowDebug = false; ///< Flag to show the navigation agent debug.

        /**
         * @brief Finds a path from the start to the end.
         * @param start The start position.
         * @param end The end position.
         * @return The path.
         */
        std::vector<glm::vec3> FindPath(const glm::vec3 start, const glm::vec3 end) const { return m_PathFinder->FindPath(start, end); }

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
        void SetNavMeshComponent(const Ref<NavMeshComponent>& navMeshComponent) { m_NavMeshComponent = navMeshComponent; }

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::make_nvp("NavMeshComponent", m_NavMeshComponent));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::make_nvp("NavMeshComponent", m_NavMeshComponent));

            m_PathFinder = CreateRef<NavMeshPathfinding>(m_NavMeshComponent->GetNavMesh());
        }

    private:
        Ref<NavMeshPathfinding> m_PathFinder = nullptr; ///< The pathfinder.
        Ref<NavMeshComponent> m_NavMeshComponent = nullptr; ///< The navigation mesh component.
    };
 }
 
 /** @} */
