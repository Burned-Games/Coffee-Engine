#include "SceneTree.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <tracy/Tracy.hpp>

namespace Coffee {

    void HierarchyComponent::FixNode(entt::registry& registry, entt::entity previous)
    {
        // fix m_Prev pointing to the same entity this component is attached to
        m_Prev = previous;

        // Iterate children and fix recursively
        entt::entity currentChild = m_First;
        entt::entity previousChild = entt::null;
        while (currentChild != entt::null)
        {
            auto& hierarchyComponent = registry.get<HierarchyComponent>(currentChild);

            hierarchyComponent.FixNode(registry, previousChild);

            previousChild = currentChild;
            currentChild = hierarchyComponent.m_Next;
        }
    }

    HierarchyComponent::HierarchyComponent(entt::entity parent)
    {
        m_Parent = parent;
        m_First = entt::null;
        m_Next = entt::null;
        m_Prev = entt::null;
    }
    HierarchyComponent::HierarchyComponent()
    {
        m_Parent = entt::null;
        m_First = entt::null;
        m_Next = entt::null;
        m_Prev = entt::null;
    }

    void HierarchyComponent::OnConstruct(Scene* scene, entt::registry& registry, entt::entity entity)
    {
        if (scene and scene->IsLoading()) return;

        auto& hierarchy = registry.get<HierarchyComponent>(entity);

        if(hierarchy.m_Parent != entt::null)
        {
            auto& parentHierarchy = registry.get<HierarchyComponent>(hierarchy.m_Parent);

            if(parentHierarchy.m_First == entt::null)
            {
                parentHierarchy.m_First = entity;
            }
            else
            {
                //Get the last child of the parent
                auto lastEntity = parentHierarchy.m_First;
                auto lastHierarchy = registry.try_get<HierarchyComponent>(lastEntity);
                while(lastHierarchy != nullptr && lastHierarchy->m_Next != entt::null)
                {
                    auto nextEntity = lastHierarchy->m_Next;
                    auto nextHierarchy = registry.try_get<HierarchyComponent>(nextEntity);

                    if(nextHierarchy == nullptr)
                    {
                        break;
                    }

                    lastEntity = nextEntity;
                    lastHierarchy = nextHierarchy;
                }
                if (lastEntity == entity)
                {
                    return;
                }
                lastHierarchy->m_Next = entity;
                hierarchy.m_Prev = lastEntity;
            }
        }
    }

    void HierarchyComponent::OnDestroy(entt::registry& registry, entt::entity entity)
    {
        auto& hierarchy = registry.get<HierarchyComponent>(entity);
        // if is the first child
        if(hierarchy.m_Prev == entt::null || !registry.valid(hierarchy.m_Prev))
        {
            if(hierarchy.m_Parent != entt::null && registry.valid(hierarchy.m_Parent))
            {
                auto parent_hierarchy = registry.try_get<HierarchyComponent>(hierarchy.m_Parent);
                if(parent_hierarchy != nullptr)
                {
                    parent_hierarchy->m_First = hierarchy.m_Next;
                    if(hierarchy.m_Next != entt::null)
                    {
                        auto next_hierarchy = registry.try_get<HierarchyComponent>(hierarchy.m_Next);
                        if(next_hierarchy != nullptr)
                        {
                            next_hierarchy->m_Prev = entt::null;
                        }
                    }
                }
            }
        }
        else
        {
            auto prev_hierarchy = registry.try_get<HierarchyComponent>(hierarchy.m_Prev);
            if(prev_hierarchy != nullptr)
            {
                prev_hierarchy->m_Next = hierarchy.m_Next;
            }
            if(hierarchy.m_Next != entt::null)
            {
                auto next_hierarchy = registry.try_get<HierarchyComponent>(hierarchy.m_Next);
                if(next_hierarchy != nullptr)
                {
                    next_hierarchy->m_Prev = hierarchy.m_Prev;
                }
            }
        }
    }
    void HierarchyComponent::OnUpdate(entt::registry& registry, entt::entity entity)
    {
        
    }

    void HierarchyComponent::Reparent(entt::registry& registry, entt::entity entity, entt::entity parent)
    {
        ZoneScoped;
    
        auto hierarchyComponent = registry.try_get<HierarchyComponent>(entity);

        //Prevent making a parent into a child of one of its own children
        HierarchyComponent* h = registry.try_get<HierarchyComponent>(parent);
        while (h != nullptr)
        {
            // if one of the parents iterated through is equal to the entity being re-parented, abort
            if (h->m_Parent == entity)
            {
                return;
            }
            // Get next parent up the chain and keep iterating
            h = registry.try_get<HierarchyComponent>(h->m_Parent);
        }
    
        HierarchyComponent::OnDestroy(registry, entity);
    
        hierarchyComponent->m_Parent = entt::null;
        hierarchyComponent->m_Next = entt::null;
        hierarchyComponent->m_Prev = entt::null;
    
        if (parent != entt::null) {

            hierarchyComponent->m_Parent = parent;
            HierarchyComponent::OnConstruct(nullptr, registry, entity);
    
            // Mark the entity and its children as dirty
            auto& transformComponent = registry.get<TransformComponent>(entity);
            transformComponent.MarkDirty();
        }
    }

    void HierarchyComponent::Reorder(entt::registry& registry, entt::entity entity, entt::entity after,
        entt::entity before)
    {
        ZoneScoped;

        // Can't move next to itself, the result would be the same position
        if (entity == after || entity == before)
        {
            return;
        }

        auto hierarchyComponent = registry.try_get<HierarchyComponent>(entity);
        auto afterHierarchy = registry.try_get<HierarchyComponent>(after);
        auto beforeHierarchy = registry.try_get<HierarchyComponent>(before);

        HierarchyComponent* h = nullptr; // Set to either the entity before or after (at least one must exist, "before" has priority)
        entt::entity parentTmp = entt::null; //Prevent double assignment of parent later if both the "before" and "after" entities are valid
        if (afterHierarchy)
        {
            h = afterHierarchy;
            parentTmp = afterHierarchy->m_Parent;
        }
        else if (beforeHierarchy)
        {
            h = beforeHierarchy;
            parentTmp = beforeHierarchy->m_Parent;
        }

        if (h == nullptr)
        {
            COFFEE_ERROR("This error should never be shown (HierarchyComponent::Reorder). Both new sibling entities were null");
            return;
        }

        // Check if entity is being moved into one of its own children
        while (h != nullptr)
        {
            // if one of the parents iterated through is equal to the entity being moved, abort
            if (h->m_Parent == entity)
            {
                auto tag = registry.try_get<TagComponent>(h->m_Parent);
                COFFEE_CORE_WARN("Tried to re-parent {} to its own child. This is not allowed", tag ? tag->Tag : "an entity");
                return;
            }
            // Get next parent up the chain and keep iterating
            h = registry.try_get<HierarchyComponent>(h->m_Parent);
        }

        // Remove temporalilly from children list
        HierarchyComponent::OnDestroy(registry, entity);

        hierarchyComponent->m_Parent = parentTmp;
        hierarchyComponent->m_Next = entt::null;
        hierarchyComponent->m_Prev = entt::null;

        // Might need a diagram to properly understand this part (^~^')
        // Place entity between afterHierarchy and its next sibling (if any)
        if (afterHierarchy != nullptr)
        {
            auto oldNext = afterHierarchy->m_Next;
            if (oldNext != entt::null)
            {
                auto oldNextComponent = registry.try_get<HierarchyComponent>(oldNext);
                oldNextComponent->m_Prev = entity;
                hierarchyComponent->m_Next = oldNext;
            }
            afterHierarchy->m_Next = entity;
            hierarchyComponent->m_Prev = after;

        }
        // Place entity between beforeHierarchy and its previous sibling (if any)
        else if (beforeHierarchy != nullptr)
        {
            auto oldPrev = beforeHierarchy->m_Prev;
            if (oldPrev != entt::null && oldPrev != entity)
            {
                auto oldPrevComponent = registry.try_get<HierarchyComponent>(oldPrev);
                oldPrevComponent->m_Next = entity;
                hierarchyComponent->m_Prev = oldPrev;
            }
            beforeHierarchy->m_Prev = entity;
            hierarchyComponent->m_Next = before;

            // If the moved entity ends up being the first in the child list, update the parent accordingly
            if (hierarchyComponent->m_Prev == entt::null && parentTmp != entt::null)
            {
                auto parent = registry.try_get<HierarchyComponent>(parentTmp);
                parent->m_First = entity;
            }
        }

    }

    SceneTree::SceneTree(Scene* scene) : m_Context(scene)
    {
        auto& registry = m_Context->m_Registry;
        registry.on_construct<HierarchyComponent>().connect<&HierarchyComponent::OnConstruct>(m_Context);
        registry.on_update<HierarchyComponent>().connect<&HierarchyComponent::OnUpdate>();
        registry.on_destroy<HierarchyComponent>().connect<&HierarchyComponent::OnDestroy>();
    }

    void SceneTree::Update()
    {
        ZoneScoped;
        auto& registry = m_Context->m_Registry;
        auto view = registry.view<TransformComponent, ActiveComponent>();
        for (auto entity : view) {
    
            const auto transform = view.get<TransformComponent>(entity);
            if (transform.IsDirty()) {
                UpdateTransform(entity);
            }
        }
    }

    void SceneTree::UpdateTransform(entt::entity entity)
    {
        auto& registry = m_Context->m_Registry;
        auto view = registry.view<TransformComponent, HierarchyComponent>();

        auto& hierarchyComponent = view.get<HierarchyComponent>(entity);
        auto& transformComponent = view.get<TransformComponent>(entity);

        // Update the world transform of the entity
        if (hierarchyComponent.m_Parent != entt::null)
        {
            auto& parentTransformComponent = view.get<TransformComponent>(hierarchyComponent.m_Parent);
            
            transformComponent.SetWorldTransform(parentTransformComponent.GetWorldTransform());
        } 
        else
        {
            transformComponent.SetWorldTransform(glm::mat4(1.0f));
        }
    
        // Recursively update all the children
        entt::entity child = hierarchyComponent.m_First;
        while (child != entt::null) {
            UpdateTransform(child);
            child = view.get<HierarchyComponent>(child).m_Next;
        }
    }

}