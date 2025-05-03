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

        for (auto& item : s_SortedUIItems)
        {
            entt::entity entity = item.Entity;

            if (!registry.any_of<ActiveComponent>(entity)) continue;

            switch (item.ComponentType) {
            case UIComponentType::Image:
                RenderUIImage(registry, item);
                break;
            case UIComponentType::Text:
                RenderUIText(registry, item);
                break;
            case UIComponentType::Toggle:
                RenderUIToggle(registry, item);
                break;
            case UIComponentType::Button:
                RenderUIButton(registry, item);
                break;
            case UIComponentType::Slider:
                RenderUISlider(registry, item);
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
            item.TransformDirty = true;
            item.ParentSizeDirty = true;
            item.WorldTransform = glm::mat4(1.0f);
            item.ParentSize = glm::vec2(0.0f);

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

        for (auto& item : s_SortedUIItems)
        {
            item.TransformDirty = true;
            item.ParentSizeDirty = true;
        }
    }

    void UIManager::RenderUIImage(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& uiImageComponent = registry.get<UIImageComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, uiImageComponent.Anchor, item);

        if (item.TransformDirty || HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));

            float rotation = transformComponent.GetLocalRotation().z;

            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

            item.TransformDirty = false;
        }

        Renderer2D::DrawQuad(item.WorldTransform, uiImageComponent.Texture, 1.0f, uiImageComponent.Color, Renderer2D::RenderMode::Screen, (uint32_t)entity, uiImageComponent.UVRect);
    }

    void UIManager::RenderUIText(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& uiTextComponent = registry.get<UITextComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        if (uiTextComponent.Text.empty())
            return;

        if (!uiTextComponent.UIFont)
            uiTextComponent.UIFont = Font::GetDefault();

        auto anchored = CalculateAnchoredTransform(registry, entity, uiTextComponent.Anchor, item);

        if (item.TransformDirty || HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(1.0f));

            float rotation = transformComponent.GetLocalRotation().z;

            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(1.0f, -1.0f, 1.0f));

            item.TransformDirty = false;
        }

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

        Renderer2D::DrawTextString(uiTextComponent.Text, uiTextComponent.UIFont, item.WorldTransform, textParams, Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUIToggle(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& toggleComponent = registry.get<UIToggleComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, toggleComponent.Anchor, item);

        if (item.TransformDirty || HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));

            float rotation = transformComponent.GetLocalRotation().z;
            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

            item.TransformDirty = false;
        }

        Ref<Texture2D> currentTexture = toggleComponent.Value ? toggleComponent.OnTexture : toggleComponent.OffTexture;

        if (currentTexture)
            Renderer2D::DrawQuad(item.WorldTransform, currentTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUIButton(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& button = registry.get<UIButtonComponent>(entity);
        auto& transform = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, button.Anchor, item);

        if (item.TransformDirty || HasTransformChanged(entity, anchored))
        {
            transform.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transform.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));

            float rotation = transform.GetLocalRotation().z;
            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

            item.TransformDirty = false;
        }

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
            Renderer2D::DrawQuad(item.WorldTransform, currentTexture, 1.0f, currentColor, Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUISlider(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& sliderComponent = registry.get<UISliderComponent>(entity);
        auto& transformComponent = registry.get<TransformComponent>(entity);

        auto anchored = CalculateAnchoredTransform(registry, entity, sliderComponent.Anchor, item);

        float rotation = transformComponent.GetLocalRotation().z;

        if (item.TransformDirty || HasTransformChanged(entity, anchored))
        {
            transformComponent.SetLocalPosition(glm::vec3(anchored.Position, 0.0f));
            transformComponent.SetLocalScale(glm::vec3(anchored.Size.x, anchored.Size.y, 1.0f));

            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

            item.TransformDirty = false;
        }

        if (sliderComponent.BackgroundTexture)
            Renderer2D::DrawQuad(item.WorldTransform, sliderComponent.BackgroundTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);

        float normalizedValue = (sliderComponent.Value - sliderComponent.MinValue) / (sliderComponent.MaxValue - sliderComponent.MinValue);
        normalizedValue = glm::clamp(normalizedValue, 0.0f, 1.0f);

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

    glm::vec2 UIManager::GetParentSize(entt::registry& registry, UIRenderItem& item)
    {
        if (item.Parent == entt::null)
        {
            item.ParentSize = WindowSize;
            item.ParentSizeDirty = false;
            return WindowSize;
        }

        if (!item.ParentSizeDirty)
            return item.ParentSize;

        for (auto& parentItem : s_SortedUIItems)
        {
            if (parentItem.Entity == item.Parent)
            {
                if (parentItem.ParentSizeDirty)
                {
                    glm::vec2 parentParentSize = GetParentSize(registry, parentItem);

                    RectAnchor* parentAnchor = nullptr;

                    if (registry.any_of<UIImageComponent>(item.Parent))
                        parentAnchor = &registry.get<UIImageComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UITextComponent>(item.Parent))
                        parentAnchor = &registry.get<UITextComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UIButtonComponent>(item.Parent))
                        parentAnchor = &registry.get<UIButtonComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UIToggleComponent>(item.Parent))
                        parentAnchor = &registry.get<UIToggleComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UISliderComponent>(item.Parent))
                        parentAnchor = &registry.get<UISliderComponent>(item.Parent).Anchor;

                    if (parentAnchor)
                    {
                        bool isStretchedX = (parentAnchor->AnchorMin.x != parentAnchor->AnchorMax.x);
                        bool isStretchedY = (parentAnchor->AnchorMin.y != parentAnchor->AnchorMax.y);

                        glm::vec2 offsetSize = parentAnchor->OffsetMax - parentAnchor->OffsetMin;

                        glm::vec2 finalSize;
                        finalSize.x = isStretchedX ? parentParentSize.x * (parentAnchor->AnchorMax.x - parentAnchor->AnchorMin.x) + offsetSize.x : offsetSize.x;
                        finalSize.y = isStretchedY ? parentParentSize.y * (parentAnchor->AnchorMax.y - parentAnchor->AnchorMin.y) + offsetSize.y : offsetSize.y;

                        parentItem.ParentSize = parentParentSize;
                        parentItem.ParentSizeDirty = false;

                        item.ParentSize = finalSize;
                        item.ParentSizeDirty = false;
                        return finalSize;
                    }
                }
                else
                {
                    RectAnchor* parentAnchor = nullptr;

                    if (registry.any_of<UIImageComponent>(item.Parent))
                        parentAnchor = &registry.get<UIImageComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UITextComponent>(item.Parent))
                        parentAnchor = &registry.get<UITextComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UIButtonComponent>(item.Parent))
                        parentAnchor = &registry.get<UIButtonComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UIToggleComponent>(item.Parent))
                        parentAnchor = &registry.get<UIToggleComponent>(item.Parent).Anchor;
                    else if (registry.any_of<UISliderComponent>(item.Parent))
                        parentAnchor = &registry.get<UISliderComponent>(item.Parent).Anchor;

                    if (parentAnchor)
                    {
                        bool isStretchedX = (parentAnchor->AnchorMin.x != parentAnchor->AnchorMax.x);
                        bool isStretchedY = (parentAnchor->AnchorMin.y != parentAnchor->AnchorMax.y);

                        glm::vec2 offsetSize = parentAnchor->OffsetMax - parentAnchor->OffsetMin;

                        glm::vec2 finalSize;
                        finalSize.x = isStretchedX ? parentItem.ParentSize.x * (parentAnchor->AnchorMax.x - parentAnchor->AnchorMin.x) + offsetSize.x : offsetSize.x;
                        finalSize.y = isStretchedY ? parentItem.ParentSize.y * (parentAnchor->AnchorMax.y - parentAnchor->AnchorMin.y) + offsetSize.y : offsetSize.y;

                        item.ParentSize = finalSize;
                        item.ParentSizeDirty = false;
                        return finalSize;
                    }
                }
                break;
            }
        }

        item.ParentSize = WindowSize;
        item.ParentSizeDirty = false;
        return WindowSize;
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

    UIManager::AnchoredTransform UIManager::CalculateAnchoredTransform(entt::registry& registry, entt::entity entity, const RectAnchor& anchor, UIRenderItem& item)
    {
        AnchoredTransform result;

        glm::vec2 referenceSize = CanvasReferenceSize;
        glm::vec2 parentSize = referenceSize;
        glm::vec2 parentPosition = glm::vec2(WindowSize.x / 2, WindowSize.y / 2);
        bool hasParent = false;

        if (registry.any_of<HierarchyComponent>(entity))
        {
            auto& hierarchy = registry.get<HierarchyComponent>(entity);
            if (hierarchy.m_Parent != entt::null)
            {
                hasParent = true;
                parentSize = GetParentSize(registry, item);

                if (registry.any_of<TransformComponent>(hierarchy.m_Parent) && !registry.any_of<UIComponent>(hierarchy.m_Parent))
                {
                    auto& parentTransform = registry.get<TransformComponent>(hierarchy.m_Parent);
                    parentPosition = glm::vec2(parentTransform.GetLocalPosition());
                }
                else
                {
                    hasParent = false;
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

            left = WindowSize.x * anchor.AnchorMin.x + anchor.OffsetMin.x * UIScale;
            top = WindowSize.y * anchor.AnchorMin.y + anchor.OffsetMin.y * UIScale;
            right = WindowSize.x * anchor.AnchorMax.x + anchor.OffsetMax.x * UIScale;
            bottom = WindowSize.y * anchor.AnchorMax.y + anchor.OffsetMax.y * UIScale;

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

    UIManager::UIRenderItem UIManager::GetUIRenderItem(entt::entity entity)
    {
        for (const auto& item : s_SortedUIItems)
        {
            if (item.Entity == entity)
                return item;
        }

        return {};
    }

} // Coffee