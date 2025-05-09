#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "Panel.h"
#include "entt/entity/fwd.hpp"

namespace Coffee {

    class SceneTreePanel : public Panel
    {
    public:
        SceneTreePanel() = default;
        SceneTreePanel(const Ref<Scene>& scene);

        void SetContext(const Ref<Scene>& scene);

        void OnImGuiRender() override;

        Entity GetSelectedEntity() const { return m_SelectionContext; };
        void SetSelectedEntity(Entity entity) { m_SelectionContext = entity; };

        void CreatePrefab(Entity entity);

    private:
        void DrawEntityNode(Entity entity, bool drawChildren = true);
        void DrawComponents(Entity entity);

        //UI functions for scenetree menus
        void ShowCreateEntityMenu();
        bool ResizeColliderToFitMeshAABB(Entity entity, RigidbodyComponent& rbComponent);

        void DrawTransform(TransformComponent& transformComponent);
        void DrawUITransform(TransformComponent& transformComponent, RectAnchor& anchor, Entity entity);

    private:
        Ref<Scene> m_Context;
        Entity m_SelectionContext;

        bool m_ShowLuaScriptOptions = false;
    };

}