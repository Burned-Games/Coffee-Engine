#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Layer.h"
#include "CoffeeEngine/Events/ApplicationEvent.h"
#include "CoffeeEngine/Events/KeyEvent.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Renderer/RenderTarget.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "Panels/ContentBrowserPanel.h"
#include "Panels/MonitorPanel.h"
#include "Panels/SceneTreePanel.h"
#include "Panels/ProjectSettingsPanel.h"

#include <Panels/ImportPanel.h>
#include <Panels/OutputPanel.h>

namespace Coffee {

    class EditorLayer : public Coffee::Layer
    {
    public:
        EditorLayer();
        virtual ~EditorLayer() = default;

        void OnAttach() override;

        void OnUpdate(float dt) override;

        void OnEvent(Event& event) override;

        bool OnKeyPressed(KeyPressedEvent& event);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
        bool OnFileDrop(FileDropEvent& event);

        void OnDetach() override;

        void OnImGuiRender() override;
    private:
        void OnOverlayRender();
        void ResizeViewport(float width, float height);

        // Editor State
        void OnScenePlay();
        void OnScenePause();
        void OnSceneStop();

        //Project Management
        void NewProject();
        void OpenProject();
        void SaveProject();

        //Scene Management
        void NewScene();
        void OpenScene();
        void SaveScene();
        void SaveSceneAs();
    private:
        RenderTarget* m_ViewportRenderTarget;

        Ref<Scene> m_EditorScene;
        Ref<Scene> m_ActiveScene;

        EditorCamera m_EditorCamera;

        bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
        glm::vec2 m_ViewportBounds[2];

        int m_GizmoType = -1;

        //Panels
        SceneTreePanel m_SceneTreePanel;
        ContentBrowserPanel m_ContentBrowserPanel;
        OutputPanel m_OutputPanel;
        MonitorPanel m_MonitorPanel;
        ImportPanel m_ImportPanel;
        ProjectSettingsPanel m_ProjectSettingsPanel;
    };

}
