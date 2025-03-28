#pragma once

#include "CoffeeEngine/Core/Assert.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <cstdint>
#include <utility>
#include <vector>

namespace Coffee {

    /**
     * @defgroup scene Scene
     * @{
     */

    /**
     * @brief Class representing an entity in the scene.
     * @ingroup scene
     */
    class Entity
    {
    public:
        /**
         * @brief Default constructor for Entity.
         */
        Entity() = default;

        /**
         * @brief Constructor for Entity with handle and scene.
         * @param handle The entity handle.
         * @param scene The scene the entity belongs to.
         */
        Entity(entt::entity handle, Scene* scene);

        /**
         * @brief Copy constructor for Entity.
         * @param other The other entity to copy from.
         */
        Entity(const Entity& other) = default;

        /**
         * @brief Add a component to the entity.
         * @tparam T The component type.
         * @tparam Args The component constructor arguments.
         * @param args The component constructor arguments.
         * @return Reference to the added component.
         */
        template<typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            COFFEE_CORE_ASSERT(!HasComponent<T>(), "Entity already has this component!");
            
            if constexpr(std::is_empty_v<T>) { // Check if the component is empty (ActiveComponent, for example)
                // For empty types, register the component and return a static instance
                m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);

                static T emptyInstance; // Since empty types have no state, a static instance is functionally equivalent
                return emptyInstance;
            } else {
                return m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Get a component from the entity.
         * @tparam T The component type.
         * @return Reference to the component.
         */
        template<typename T>
        T& GetComponent()
        {
            COFFEE_CORE_ASSERT(HasComponent<T>(), "Entity does not have this component!");
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        /**
         * @brief Check if the entity has a component.
         * @tparam T The component type.
         * @return True if the entity has the component, false otherwise.
         */
        template<typename T>
        bool HasComponent()
        {
            return m_Scene->m_Registry.all_of<T>(m_EntityHandle);
        }

        /**
         * @brief Remove a component from the entity.
         * @tparam T The component type.
         */
        template<typename T>
        void RemoveComponent()
        {
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        bool IsValid() const 
        {
            return m_Scene->m_Registry.valid(m_EntityHandle);
        }

        /**
         * @brief Check if the entity is valid.
         * @return True if the entity is valid, false otherwise.
         */
        operator bool() const { return m_EntityHandle != entt::null; }

        /**
         * @brief Convert the entity to its handle.
         * @return The entity handle.
         */
        operator entt::entity() const { return m_EntityHandle; }

        /**
         * @brief Convert the entity to its handle as a uint32_t.
         * @return The entity handle as a uint32_t.
         */
        operator uint32_t() const { return (uint32_t)m_EntityHandle; }

        /**
         * @brief Equality operator.
         * @param other The other entity to compare with.
         * @return True if the entities are equal, false otherwise.
         */
        bool operator==(const Entity& other) const { return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene; }

        /**
         * @brief Inequality operator.
         * @param other The other entity to compare with.
         * @return True if the entities are not equal, false otherwise.
         */
        bool operator!=(const Entity& other) const { return !operator==(other); }

        /**
         * @brief Set the parent of the entity.
         * @param entity The parent entity.
         */
        void SetParent(Entity entity) 
        {
           HierarchyComponent::Reparent(m_Scene->m_Registry, m_EntityHandle, entity);
        }

        Entity GetParent()
        {
            const auto& hierarchyComponent = GetComponent<HierarchyComponent>();

            return Entity{ hierarchyComponent.m_Parent, m_Scene };
        }

        Entity GetNextSibling()
        {
            const auto& hierarchyComponent = GetComponent<HierarchyComponent>();

            return Entity{ hierarchyComponent.m_Next, m_Scene };
        }

        Entity GetPrevSibling()
        {
            const auto& hierarchyComponent = GetComponent<HierarchyComponent>();

            return Entity{ hierarchyComponent.m_Prev, m_Scene };
        }

        Entity GetChild(int index)
        {
            const auto& hierarchyComponent = GetComponent<HierarchyComponent>();

            entt::entity curr = hierarchyComponent.m_First;
            for (int i = 0; i < index; i++)
            {
                if (curr == entt::null)
                    return Entity{ entt::null, m_Scene };

                curr = m_Scene->m_Registry.get<HierarchyComponent>(curr).m_Next;
            }

            return Entity{ curr, m_Scene };
        }

        //Get All Children

        std::vector<Entity> GetChildren()
        {
            std::vector<Entity> children;

            const auto& hierarchyComponent = GetComponent<HierarchyComponent>();

            entt::entity curr = hierarchyComponent.m_First;
            while (curr != entt::null)
            {
                children.push_back(Entity{ curr, m_Scene });
                curr = m_Scene->m_Registry.get<HierarchyComponent>(curr).m_Next;
            }

            return children;
        }

        bool IsActive()
        {
            if (!HasComponent<ActiveComponent>())
                return false;
                
            Entity parent = GetParent();
            while (parent)
            {
                if (!parent.HasComponent<ActiveComponent>())
                    return false;
                parent = parent.GetParent();
            }
            
            return true;
        }
        
        void SetActive(bool active)
        {
            bool currentlyActive = HasComponent<ActiveComponent>();

            if (active == currentlyActive)
                return;

            if (active) {
                AddComponent<ActiveComponent>();

                // TODO move this to a more appropriate place
                // Re-enable physics if entity has a rigidbody
                if (HasComponent<RigidbodyComponent>()) {
                    auto& rbComponent = GetComponent<RigidbodyComponent>();
                    if (rbComponent.rb && rbComponent.rb->GetNativeBody()) {
                        m_Scene->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                        rbComponent.rb->GetNativeBody()->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>(m_EntityHandle)));
                    }
                }
            }
            else
            {
                // TODO move this to a more appropriate place
                // Disable physics before removing ActiveComponent
                if (HasComponent<RigidbodyComponent>()) {
                    auto& rbComponent = GetComponent<RigidbodyComponent>();
                    if (rbComponent.rb && rbComponent.rb->GetNativeBody()) {
                        m_Scene->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());
                    }
                }
                
                RemoveComponent<ActiveComponent>();
            }
            
            auto children = GetChildren();
            for (auto& child : children)
            {
                child.SetActive(active);
            }
        }

    private:
        entt::entity m_EntityHandle{ entt::null };
        Scene* m_Scene = nullptr;
    };

    /** @} */ // end of scene group

}