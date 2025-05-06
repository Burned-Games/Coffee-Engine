#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "entt/entity/fwd.hpp"
#include <cereal/cereal.hpp>
#include <entt/entt.hpp>

namespace Coffee {

    class Scene;

    /**
     * @defgroup scene Scene
     * @{
     */

    /**
     * @brief Component for managing entity hierarchy.
     * @ingroup scene
     */
    struct HierarchyComponent
    {
        // TODO delete this function once it's not needed anymore
        void FixNode(entt::registry& registry, entt::entity previous);

        /**
         * @brief Constructor with parent entity.
         * @param parent The parent entity.
         */
        HierarchyComponent(entt::entity parent);

        /**
         * @brief Default constructor.
         */
        HierarchyComponent();

        /**
         * @brief Called when the component is constructed.
         * @param registry The entity registry.
         * @param entity The entity.
         */
        static void OnConstruct(Scene* scene, entt::registry& registry, entt::entity entity);

        /**
         * @brief Called when the component is destroyed.
         * @param registry The entity registry.
         * @param entity The entity.
         */
        static void OnDestroy(entt::registry& registry, entt::entity entity);

        /**
         * @brief Called when the component is updated.
         * @param registry The entity registry.
         * @param entity The entity.
         */
        static void OnUpdate(entt::registry& registry, entt::entity entity);

        /**
         * @brief Reparent the entity to a new parent.
         * @param registry The entity registry.
         * @param entity The entity to reparent.
         * @param parent The new parent entity.
         */
        static void Reparent(entt::registry& registry, entt::entity entity, entt::entity parent);

        /**
         * @brief Move an entity within the hierarchy. Will reparent if needed
         * @param registry The entity registry
         * @param entity The entity to be moved
         * @param after The entity after which the entity will be moved to
         * @param before The entity before which the entity will be moved to
         */
        static void Reorder(entt::registry& registry, entt::entity entity, entt::entity after, entt::entity before);

        entt::entity m_Parent;
        entt::entity m_First;
        entt::entity m_Next;
        entt::entity m_Prev;

        /**
         * @brief Serialize the component.
         * @tparam Archive The archive type.
         * @param archive The archive.
         */
        template<class Archive>
/*         void serialize(Archive& archive)
        {
            archive(cereal::make_nvp("Parent", m_Parent), cereal::make_nvp("First", m_First), cereal::make_nvp("Next", m_Next), cereal::make_nvp("Prev", m_Prev));
        } */
        void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("Parent", m_Parent), cereal::make_nvp("First", m_First), cereal::make_nvp("Next", m_Next), cereal::make_nvp("Prev", m_Prev));
        }

        template<class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("Parent", m_Parent), cereal::make_nvp("First", m_First), cereal::make_nvp("Next", m_Next), cereal::make_nvp("Prev", m_Prev));
        }
    };

    /**
     * @brief Class for managing the scene tree.
     * @ingroup scene
     */
    class SceneTree
    {
    public:
        /**
         * @brief Constructor for SceneTree.
         * @param scene The scene context.
         */
        SceneTree(Scene* scene);

        /**
         * @brief Default destructor.
         */
        ~SceneTree() = default;

        /**
         * @brief Update the scene tree.
         */
        void Update();

        /**
         * @brief Update the transform of an entity.
         * @param entity The entity to update.
         */
        void UpdateTransform(entt::entity entity);

    private:
        Scene* m_Context;
    };

    /** @} */ // end of scene group
}