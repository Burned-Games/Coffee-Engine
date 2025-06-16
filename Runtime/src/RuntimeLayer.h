#pragma once

#include "CoffeeEngine/Core/Layer.h"
#include "CoffeeEngine/Events/KeyEvent.h"
#include "CoffeeEngine/Events/MouseEvent.h"
#include "CoffeeEngine/Renderer/RenderTarget.h"

namespace Coffee {

    class RuntimeLayer : public Coffee::Layer
    {
    public:
        RuntimeLayer();
        virtual ~RuntimeLayer() = default;

        void OnAttach() override;

        void OnUpdate(float dt) override;

        void OnEvent(Event& event) override;

        bool OnKeyPressed(KeyPressedEvent& event);
        bool OnMouseButtonPressed(MouseButtonPressedEvent& event);

        void OnDetach() override;
    private:
        void ResizeViewport(float width, float height);

    private:
        Ref<RenderTarget> m_ViewportRenderTarget = nullptr;


        bool m_ViewportFocused = false, m_ViewportHovered = false;
		glm::vec2 m_ViewportSize = { 0.0f, 0.0f };
        glm::vec2 m_ViewportBounds[2];
    };

}
