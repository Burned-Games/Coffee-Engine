#include "UIManager.h"

#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/SceneTree.h"

namespace Coffee {

    glm::vec2 UIManager::WindowSize;
    bool UIManager::s_NeedsSorting = true;
    std::vector<UIManager::UIRenderItem> UIManager::s_SortedUIItems;
    std::unordered_map<entt::entity, UIManager::AnchoredTransform> UIManager::s_LastTransforms;

    glm::vec2 UIManager::CanvasReferenceSize = { 1920.0f, 1080.0f };
    float UIManager::UIScale = 1.0f;

    void UIManager::UpdateUI(entt::registry& registry)
    {
        WindowSize = Renderer::GetCurrentRenderTarget()->GetSize();
        CalculateUIScaleFactor();

        if (s_NeedsSorting)
        {
            SortUIElements(registry);
            s_NeedsSorting = false;
        }

        for (const auto& item : s_SortedUIItems)
        {
            entt::entity entity = item.Entity;

            if (!registry.any_of<ActiveComponent>(entity)) continue;

            switch (item.ComponentType) {
            case UIComponentType::Image:
                RenderUIImage(registry, entity);
                break;
            case UIComponentType::Text:
                RenderUIText(registry, entity);
                break;
            case UIComponentType::Toggle:
                RenderUIToggle(registry, entity);
                break;
            case UIComponentType::Button:
                RenderUIButton(registry, entity);
                break;
            case UIComponentType::Slider:
                RenderUISlider(registry, entity);
                break;
            }
        }
    }

    template<typename T, UIManager::UIComponentType Type>
    void AddUIItems(entt::registry& registry, std::vector<UIManager::UIRenderItem>& items) {
        auto view = registry.view<T, TransformComponent>();
        for (auto entity : view) {
            auto& uiComponent = view.template get<T>(entity);

            UIManager::UIRenderItem item;
            item.Entity = entity;
            item.Layer = uiComponent.Layer;
            item.ComponentType = Type;

            if (registry.any_of<HierarchyComponent>(entity)) {
                auto& hierarchy = registry.get<HierarchyComponent>(entity);
                item.Parent = hierarchy.m_Parent;
                item.Next = hierarchy.m_Next;
            }

            items.push_back(item);
        }
    }

    void UIManager::SortUIElements(entt::registry& registry)
    {
        s_SortedUIItems.clear();

        AddUIItems<UIImageComponent, UIComponentType::Image>(registry, s_SortedUIItems);
        AddUIItems<UITextComponent, UIComponentType::Text>(registry, s_SortedUIItems);
        AddUIItems<UIToggleComponent, UIComponentType::Toggle>(registry, s_SortedUIItems);
        AddUIItems<UIButtonComponent, UIComponentType::Button>(registry, s_SortedUIItems);
        AddUIItems<UISliderComponent, UIComponentType::Slider>(registry, s_SortedUIItems);

        std::sort(s_SortedUIItems.begin(), s_SortedUIItems.end(), [&registry](const UIRenderItem& a, const UIRenderItem& b) {
            if (a.Layer != b.Layer)
                return a.Layer < b.Layer;

            if (a.Parent == b.Parent)
            {
                entt::entity current = a.Entity;
                while (current != entt::null)
                {
                    if (current == b.Entity)
                        return true;

                    if (registry.any_of<HierarchyComponent>(current))
                        current = registry.get<HierarchyComponent>(current).m_Next;
                }

                return false;
            }

            {
                entt::entity parent = b.Parent;
                while (parent != entt::null)
                {
                    if (parent == a.Entity)
                        return true;

                    if (registry.any_of<HierarchyComponent>(parent))
                        parent = registry.get<HierarchyComponent>(parent).m_Parent;
                }
            }

            {
                entt::entity parent = a.Parent;
                while (parent != entt::null)
                {
                    if (parent == b.Entity)
                        return false;

                    if (registry.any_of<HierarchyComponent>(parent))
                        parent = registry.get<HierarchyComponent>(parent).m_Parent;
                }
            }

            return static_cast<uint32_t>(a.Entity) < static_cast<uint32_t>(b.Entity);
        });
    }

    void UIManager::RenderUIImage(entt::registry& registry, entt::entity entity)
    {
        auto& uiImageComponent = registry.get<UIImageComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, uiImageComponent.Anchor, WindowSize);

        if (HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));
        }

        float rotation = transformComponent.GetLocalRotation().z;

        glm::mat4 worldTransform = glm::mat4(1.0f);
        worldTransform = glm::translate(worldTransform, glm::vec3(anchored.Position, 0.0f));
        worldTransform = glm::rotate(worldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        worldTransform = glm::scale(worldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

        Renderer2D::DrawQuad(worldTransform, uiImageComponent.Texture, 1.0f, uiImageComponent.Color, Renderer2D::RenderMode::Screen, (uint32_t)entity, uiImageComponent.UVRect);
    }

    void UIManager::RenderUIText(entt::registry& registry, entt::entity entity)
    {
        auto& uiTextComponent = registry.get<UITextComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        if (uiTextComponent.Text.empty())
            return;

        if (!uiTextComponent.UIFont)
            uiTextComponent.UIFont = Font::GetDefault();

        auto anchored = CalculateAnchoredTransform(registry, entity, uiTextComponent.Anchor, WindowSize);

        if (HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(1.0f));
        }

        float rotation = transformComponent.GetLocalRotation().z;

        glm::mat4 textTransform = glm::mat4(1.0f);
        textTransform = glm::translate(textTransform, glm::vec3(anchored.Position, 0.0f));
        textTransform = glm::rotate(textTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        textTransform = glm::scale(textTransform, glm::vec3(1.0f, -1.0f, 1.0f));

        float scaledFontSize = uiTextComponent.FontSize;

        scaledFontSize *= UIScale;

        const float minFontSize = 8.0f;
        scaledFontSize = std::max(scaledFontSize, minFontSize);

        Renderer2D::TextParams textParams;
        textParams.Color = uiTextComponent.Color;
        textParams.Kerning = uiTextComponent.Kerning;
        textParams.LineSpacing = uiTextComponent.LineSpacing;
        textParams.Size = scaledFontSize;
        textParams.Alignment = uiTextComponent.Alignment;

        Renderer2D::DrawTextString(uiTextComponent.Text, uiTextComponent.UIFont, textTransform, textParams, Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUIToggle(entt::registry& registry, entt::entity entity)
    {
        auto& toggleComponent = registry.get<UIToggleComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, toggleComponent.Anchor, WindowSize);

        if (HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));
        }

        float rotation = transformComponent.GetLocalRotation().z;
        glm::mat4 worldTransform = glm::mat4(1.0f);
        worldTransform = glm::translate(worldTransform, glm::vec3(anchored.Position, 0.0f));
        worldTransform = glm::rotate(worldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        worldTransform = glm::scale(worldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

        Ref<Texture2D> currentTexture = toggleComponent.Value ? toggleComponent.OnTexture : toggleComponent.OffTexture;

        if (currentTexture)
            Renderer2D::DrawQuad(worldTransform, currentTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUIButton(entt::registry& registry, entt::entity entity)
    {
        auto& button = registry.get<UIButtonComponent>(entity);
        auto& transform = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, button.Anchor, WindowSize);

        if (HasTransformChanged(entity, anchored))
        {
            transform.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transform.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));
        }

        float rotation = transform.GetLocalRotation().z;
        glm::mat4 worldTransform = glm::mat4(1.0f);
        worldTransform = glm::translate(worldTransform, glm::vec3(anchored.Position, 0.0f));
        worldTransform = glm::rotate(worldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        worldTransform = glm::scale(worldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

        Ref<Texture2D> currentTexture = nullptr;
        glm::vec4 currentColor{1.0f};

        UIButtonComponent::State state = button.CurrentState;

        state = button.Interactable ? button.CurrentState : UIButtonComponent::State::Disabled;

        switch (state)
        {
            case UIButtonComponent::State::Normal:
                currentTexture = button.NormalTexture;
                currentColor = button.NormalColor;
                break;
            case UIButtonComponent::State::Hover:
                currentTexture = button.HoverTexture;
                currentColor = button.HoverColor;
                break;
            case UIButtonComponent::State::Pressed:
                currentTexture = button.PressedTexture;
                currentColor = button.PressedColor;
                break;
            case UIButtonComponent::State::Disabled:
                currentTexture = button.DisabledTexture;
                currentColor = button.DisabledColor;
                break;
        }

        if (currentTexture)
            Renderer2D::DrawQuad(worldTransform, currentTexture, 1.0f, currentColor, Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUISlider(entt::registry& registry, entt::entity entity)
    {
        auto& sliderComponent = registry.get<UISliderComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, sliderComponent.Anchor, WindowSize);

        if (HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));
        }

        float rotation = transformComponent.GetLocalRotation().z;

        float normalizedValue = (sliderComponent.Value - sliderComponent.MinValue) / (sliderComponent.MaxValue - sliderComponent.MinValue);
        normalizedValue = glm::clamp(normalizedValue, 0.0f, 1.0f);

        glm::mat4 backgroundTransform = glm::mat4(1.0f);
        backgroundTransform = glm::translate(backgroundTransform, glm::vec3(anchored.Position, 0.0f));
        backgroundTransform = glm::rotate(backgroundTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        backgroundTransform = glm::scale(backgroundTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

        if (sliderComponent.BackgroundTexture)
            Renderer2D::DrawQuad(backgroundTransform, sliderComponent.BackgroundTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);

        float handlePosX = anchored.Position.x - (anchored.Size.x * 0.5f) + (normalizedValue * anchored.Size.x);
        float handlePosY = anchored.Position.y;

        float baseHandleSize = anchored.Size.y;
        glm::vec2 scaledHandleSize = baseHandleSize * sliderComponent.HandleScale;

        glm::mat4 handleTransform = glm::mat4(1.0f);
        handleTransform = glm::translate(handleTransform, glm::vec3(handlePosX, handlePosY, 0.0f));
        handleTransform = glm::rotate(handleTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        handleTransform = glm::scale(handleTransform, glm::vec3(scaledHandleSize.x, -scaledHandleSize.y, 1.0f));

        if (sliderComponent.HandleTexture)
            Renderer2D::DrawQuad(handleTransform, sliderComponent.HandleTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    glm::vec2 UIManager::GetParentSize(entt::registry& registry, entt::entity parentEntity)
    {
        if (parentEntity == entt::null)
            return WindowSize;

        RectAnchor* parentAnchor = nullptr;

        if (registry.any_of<UIImageComponent>(parentEntity))
            parentAnchor = &registry.get<UIImageComponent>(parentEntity).Anchor;
        else if (registry.any_of<UITextComponent>(parentEntity))
            parentAnchor = &registry.get<UITextComponent>(parentEntity).Anchor;
        else if (registry.any_of<UIButtonComponent>(parentEntity))
            parentAnchor = &registry.get<UIButtonComponent>(parentEntity).Anchor;
        else if (registry.any_of<UIToggleComponent>(parentEntity))
            parentAnchor = &registry.get<UIToggleComponent>(parentEntity).Anchor;
        else if (registry.any_of<UISliderComponent>(parentEntity))
            parentAnchor = &registry.get<UISliderComponent>(parentEntity).Anchor;

        if (!parentAnchor)
            return WindowSize;

        glm::vec2 parentSize = WindowSize;
        if (registry.any_of<HierarchyComponent>(parentEntity))
        {
            auto& hierarchy = registry.get<HierarchyComponent>(parentEntity);
            if (hierarchy.m_Parent != entt::null)
                parentSize = GetParentSize(registry, hierarchy.m_Parent);
        }

        bool isStretchedX = (parentAnchor->AnchorMin.x != parentAnchor->AnchorMax.x);
        bool isStretchedY = (parentAnchor->AnchorMin.y != parentAnchor->AnchorMax.y);

        glm::vec2 offsetSize = parentAnchor->OffsetMax - parentAnchor->OffsetMin;

        glm::vec2 finalSize;
        finalSize.x = isStretchedX ? parentSize.x * (parentAnchor->AnchorMax.x - parentAnchor->AnchorMin.x) + offsetSize.x : offsetSize.x;
        finalSize.y = isStretchedY ? parentSize.y * (parentAnchor->AnchorMax.y - parentAnchor->AnchorMin.y) + offsetSize.y : offsetSize.y;

        return finalSize;
    }

    AnchorPreset UIManager::GetAnchorPreset(int row, int column)
    {
        // Row: 0=top, 1=middle, 2=bottom, 3=stretch
        // Column: 0=left, 1=center, 2=right, 3=stretch

        AnchorPresetY y;
        switch(row)
        {
            case 0: y = AnchorPresetY::Top; break;
            case 1: y = AnchorPresetY::Middle; break;
            case 2: y = AnchorPresetY::Bottom; break;
            case 3: y = AnchorPresetY::Stretch; break;
            default: y = AnchorPresetY::Top;
        }

        AnchorPresetX x;
        switch(column)
        {
            case 0: x = AnchorPresetX::Left; break;
            case 1: x = AnchorPresetX::Center; break;
            case 2: x = AnchorPresetX::Right; break;
            case 3: x = AnchorPresetX::Stretch; break;
            default: x = AnchorPresetX::Left;
        }

        return AnchorPreset(x, y);
    }

    UIManager::AnchoredTransform UIManager::CalculateAnchoredTransform(entt::registry& registry, entt::entity entity, const RectAnchor& anchor, const glm::vec2& windowSize)
    {
        AnchoredTransform result;

        glm::vec2 referenceSize = CanvasReferenceSize;
        glm::vec2 parentSize = referenceSize;
        glm::vec2 parentPosition = glm::vec2(windowSize.x / 2, windowSize.y / 2);
        bool hasParent = false;

        if (registry.any_of<HierarchyComponent>(entity))
        {
            auto& hierarchy = registry.get<HierarchyComponent>(entity);
            if (hierarchy.m_Parent != entt::null)
            {
                hasParent = true;
                parentSize = GetParentSize(registry, hierarchy.m_Parent);

                if (registry.any_of<TransformComponent>(hierarchy.m_Parent))
                {
                    auto& parentTransform = registry.get<TransformComponent>(hierarchy.m_Parent);
                    parentPosition = glm::vec2(parentTransform.GetLocalPosition());
                }
            }
        }

        glm::vec4 rectInParentSpace = anchor.CalculateRect(parentSize);
        result.Size = glm::vec2(rectInParentSpace.z, rectInParentSpace.w);

        if (hasParent)
        {
            float elementCenterX = rectInParentSpace.x + rectInParentSpace.z * 0.5f;
            float elementCenterY = rectInParentSpace.y + rectInParentSpace.w * 0.5f;

            glm::vec2 relativeOffset = glm::vec2(
                elementCenterX - (parentSize.x * 0.5f),
                elementCenterY - (parentSize.y * 0.5f)
            );

            relativeOffset = ScaleSize(relativeOffset);
            result.Size = ScaleSize(result.Size);

            result.Position = parentPosition + relativeOffset;
        }
        else
        {
            float left = 0.0f;
            float top = 0.0f;
            float right = 0.0f;
            float bottom = 0.0f;

            left = windowSize.x * anchor.AnchorMin.x + anchor.OffsetMin.x * UIScale;
            top = windowSize.y * anchor.AnchorMin.y + anchor.OffsetMin.y * UIScale;
            right = windowSize.x * anchor.AnchorMax.x + anchor.OffsetMax.x * UIScale;
            bottom = windowSize.y * anchor.AnchorMax.y + anchor.OffsetMax.y * UIScale;

            result.Position.x = (left + right) * 0.5f;
            result.Position.y = (top + bottom) * 0.5f;
            result.Size.x = right - left;
            result.Size.y = bottom - top;
        }

        return result;
    }

    void UIManager::MarkChildrenForUpdate(entt::entity parentEntity)
    {
        for (const auto& item : s_SortedUIItems)
        {
            if (item.Parent == parentEntity)
            {
                s_LastTransforms.erase(item.Entity);
                MarkChildrenForUpdate(item.Entity);
            }
        }
    }

    bool UIManager::HasTransformChanged(entt::entity entity, const AnchoredTransform& newTransform)
    {
        auto it = s_LastTransforms.find(entity);
        if (it == s_LastTransforms.end())
        {
            s_LastTransforms[entity] = newTransform;
            MarkChildrenForUpdate(entity);
            return true;
        }

        const auto& lastTransform = it->second;
        bool changed = !glm::epsilonEqual(lastTransform.Position.x, newTransform.Position.x, 0.001f) ||
                      !glm::epsilonEqual(lastTransform.Position.y, newTransform.Position.y, 0.001f) ||
                      !glm::epsilonEqual(lastTransform.Size.x, newTransform.Size.x, 0.001f) ||
                      !glm::epsilonEqual(lastTransform.Size.y, newTransform.Size.y, 0.001f);

        if (changed)
        {
            s_LastTransforms[entity] = newTransform;
            MarkChildrenForUpdate(entity);
        }

        return changed;
    }

    void UIManager::SetReferenceCanvasSize(const glm::vec2& referenceSize)
    {
        CanvasReferenceSize = referenceSize;
        CalculateUIScaleFactor();
    }

    void UIManager::CalculateUIScaleFactor()
    {
        float heightScale = WindowSize.y / CanvasReferenceSize.y;
        float widthScale = WindowSize.x / CanvasReferenceSize.x;

        UIScale = std::min(heightScale, widthScale);
    }

    glm::vec2 UIManager::ScaleSize(const glm::vec2& size)
    {
        return size * UIScale;
    }

    glm::vec2 UIManager::ScalePosition(const glm::vec2& position)
    {
        float normalizedX = position.x / CanvasReferenceSize.x;
        float normalizedY = position.y / CanvasReferenceSize.y;

        return { normalizedX * WindowSize.x, normalizedY * WindowSize.y };
    }

} // Coffee