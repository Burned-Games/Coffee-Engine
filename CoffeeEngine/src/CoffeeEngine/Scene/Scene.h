#pragma once

#include "CoffeeEngine/Core/DataStructures/Octree.h"
#include "CoffeeEngine/Events/Event.h"
#include "CoffeeEngine/Navigation/NavMesh.h"
#include "CoffeeEngine/Navigation/NavMeshPathfinding.h"
#include "CoffeeEngine/Physics/PhysicsWorld.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "CoffeeEngine/Scene/Components.h"
#include "entt/entity/fwd.hpp"

#include <entt/entt.hpp>
#include "entt/entity/snapshot.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace Coffee {

    /**
     * @defgroup scene Scene
     * @{
     */

    struct AnimatorComponent;
    class Entity;
    class Model;
    class Renderer2D;

    /**
     * @brief Class representing a scene.
     * @ingroup scene
     */
    class Scene
    {
    public:
        /**
         * @brief Constructor for Scene.
         */
        Scene();

        /**
         * @brief Default destructor.
         */
        ~Scene() = default;

        //Scene(Ref<Scene> other);

        /**
         * @brief Create an entity in the scene.
         * @param name The name of the entity.
         * @return The created entity.
         */
        Entity CreateEntity(const std::string& name = std::string());

        Entity Duplicate(const Entity& parent);

        /**
         * @brief Destroy an entity in the scene.
         * @param entity The entity to destroy.
         */
        void DestroyEntity(Entity entity);

        Entity GetEntityByName(const std::string& name);

        std::vector<Entity> GetAllEntities();

        template<typename... Components>
        auto GetAllEntitiesWithComponents()
        {
            return m_Registry.view<Components...>();
        }

        /**
         * @brief Initialize the scene.
         */
        void OnInitEditor();
        void OnInitRuntime();

        /**
         * @brief Update the scene in editor mode.
         * @param camera The editor camera.
         * @param dt The delta time.
         */
        void OnUpdateEditor(EditorCamera& camera, float dt);

        /**
         * @brief Update the scene in runtime mode.
         * @param dt The delta time.
         */
        void OnUpdateRuntime(float dt);

        /**
         * @brief Handle an event in the scene.
         * @param e The event.
         */
        void OnEvent(Event& e);

        /**
         * @brief Exit the scene.
         */
        void OnExitEditor();
        void OnExitRuntime();

        void OnEditorUpdateUI(float dt, entt::registry& registry);
        void OnRuntimeUpdateUI(float dt, entt::registry& registry);

        glm::vec2 CalculateAnchorOffset(UIAnchorPosition anchor, const glm::vec2& windowSize);
        std::vector<std::string> SplitTextIntoLines(const std::string& text);



        /**
         * @brief Load a scene from a file.
         * @param path The path to the file.
         * @return The loaded scene.
         */
        static Ref<Scene> Load(const std::filesystem::path& path);

        /**
         * @brief Save a scene to a file.
         * @param path The path to the file.
         * @param scene The scene to save.
         */
        static void Save(const std::filesystem::path& path, Ref<Scene> scene);

        const std::filesystem::path& GetFilePath() const { return m_FilePath; }
        void SetFilePath(const std::filesystem::path& path) { m_FilePath = path; }

        /**
         * @brief Update the positions of the audio components.
         */
        void UpdateAudioComponentsPositions();

        const std::filesystem::path& GetFilePath() { return m_FilePath; }


        /**
         * @brief Assigns animators to meshes.
         * @param animators The vector of animator components.
         */
        void AssignAnimatorsToMeshes(const std::vector<AnimatorComponent*> animators);
    
    private:
        friend class cereal::access;

        /**
         * @brief Serializes the scene to an archive.
         * @tparam Archive The type of the archive.
         * @param archive The archive to save the scene to.
         */
         template <class Archive>
         void save(Archive& archive) const
         {
            entt::snapshot{m_Registry}
            .get<entt::entity>(archive)
            .template get<TagComponent>(archive)
            .template get<TransformComponent>(archive)
            .template get<HierarchyComponent>(archive)
            .template get<CameraComponent>(archive)
            .template get<MeshComponent>(archive)
            .template get<MaterialComponent>(archive)
            .template get<LightComponent>(archive)
            .template get<RigidbodyComponent>(archive)
            .template get<ScriptComponent>(archive)
            .template get<NavMeshComponent>(archive)
            .template get<NavigationAgentComponent>(archive)
            .template get<AnimatorComponent>(archive)
            .template get<AudioSourceComponent>(archive)
            .template get<AudioListenerComponent>(archive)
            .template get<AudioZoneComponent>(archive)
            .template get<UIImageComponent>(archive)
            .template get<UICanvasComponent>(archive)
            .template get<UITextComponent>(archive)
            .template get<UISliderComponent>(archive)
            .template get<UIButtonComponent>(archive)
            .template get<UIToggleComponent>(archive)
            .template get<ParticlesSystemComponent>(archive);
         }

        /**
         * @brief Deserializes the scene from an archive.
         * @tparam Archive The type of the archive.
         * @param archive The archive to load the scene from.
         */
        template <class Archive>
        void load(Archive& archive)
        {
            entt::snapshot_loader{m_Registry}
            .get<entt::entity>(archive)
            .template get<TagComponent>(archive)
            .template get<TransformComponent>(archive)
            .template get<HierarchyComponent>(archive)
            .template get<CameraComponent>(archive)
            .template get<MeshComponent>(archive)
            .template get<MaterialComponent>(archive)
            .template get<LightComponent>(archive)
            .template get<RigidbodyComponent>(archive)
            .template get<ScriptComponent>(archive)
            .template get<NavMeshComponent>(archive)
            .template get<NavigationAgentComponent>(archive)
            .template get<AnimatorComponent>(archive)
            .template get<AudioSourceComponent>(archive)
            .template get<AudioListenerComponent>(archive)
            .template get<AudioZoneComponent>(archive)
            .template get<UIImageComponent>(archive)
            .template get<UICanvasComponent>(archive)
            .template get<UITextComponent>(archive)
            .template get<UISliderComponent>(archive)
            .template get<UIButtonComponent>(archive)
            .template get<UIToggleComponent>(archive)
            .template get<ParticlesSystemComponent>(archive);

            AssignAnimatorsToMeshes(AnimationSystem::GetAnimators());
        }

    private:
        // NOTE: this macro should be modified when adding new components
        #define ALL_COMPONENTS TagComponent, TransformComponent, HierarchyComponent, CameraComponent, MeshComponent, MaterialComponent, LightComponent, RigidbodyComponent, ScriptComponent, AudioSourceComponent, AudioListenerComponent, AudioZoneComponent, UIImageComponent, UICanvasComponent, UITextComponent, UISliderComponent, UIButtonComponent, UIToggleComponent, ParticlesSystemComponent //, AnimatorComponent

        entt::registry m_Registry;
        Scope<SceneTree> m_SceneTree;
        Octree<Ref<Mesh>> m_Octree;
        PhysicsWorld m_PhysicsWorld;


        // Temporal: Scenes should be Resources and the Base Resource class already has a path variable.
        std::filesystem::path m_FilePath;


        friend class Entity;
        friend class SceneTree;
        friend class SceneTreePanel;
        friend class CollisionSystem;
    };

    /**
     * @brief Add a model to the scene tree.
     * @param scene The scene.
     * @param model The model to add.
     */
    void AddModelToTheSceneTree(Scene* scene, Ref<Model> model, AnimatorComponent* animatorComponent = nullptr);

    /** @} */ // end of scene group
}
