#include "UIManager.h"

#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Renderer/Renderer2D.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/SceneTree.h"

namespace Coffee {

    glm::vec2 UIManager::WindowSize;
    glm::vec2 UIManager::m_lastWindowSize = { 0.0f, 0.0f };
    bool UIManager::s_NeedsSorting = true;
    std::vector<UIManager::UIRenderItem> UIManager::s_SortedUIItems;
    std::unordered_map<entt::entity, UIManager::AnchoredTransform> UIManager::s_LastTransforms;

    glm::vec2 UIManager::CanvasReferenceSize = { 1920.0f, 1080.0f };
    float UIManager::UIScale = 1.0f;

    std::unordered_map<entt::entity, std::vector<UIManager::TransformOperation>> UIManager::s_PendingTransforms;

    void UIManager::UpdateUI(entt::registry& registry)
    {
        WindowSize = Renderer::GetCurrentRenderTarget()->GetSize();

        if (WindowSize != m_lastWindowSize)
        {
            CalculateUIScaleFactor();
            for (auto& item : s_SortedUIItems)
            {
                item.TransformDirty = true;
                item.ParentSizeDirty = true;
            }
            m_lastWindowSize = WindowSize;
        }

        if (s_NeedsSorting)
        {
            SortUIElements(registry);
            s_NeedsSorting = false;
        }

        ProcessPendingTransforms(registry);

        for (auto& item : s_SortedUIItems)
        {
            if (item.Parent == entt::null)
                UpdateUITranformRecursive(registry, item);
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
            auto& transformComponent = view.template get<TransformComponent>(entity);

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
        AddUIItems<UIComponent, UIComponentType::Empty>(registry, s_SortedUIItems);

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

        Renderer2D::DrawQuad(item.WorldTransform, uiImageComponent.Texture, 1.0f, uiImageComponent.Color, Renderer2D::RenderMode::Screen, (uint32_t)entity, uiImageComponent.UVRect);
    }

    void UIManager::RenderUIText(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& uiTextComponent = registry.get<UITextComponent>(entity);

        if (uiTextComponent.Text.empty())
            return;

        if (!uiTextComponent.UIFont)
            uiTextComponent.UIFont = Font::GetDefault();

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

        Ref<Texture2D> currentTexture = toggleComponent.Value ? toggleComponent.OnTexture : toggleComponent.OffTexture;

        if (currentTexture)
            Renderer2D::DrawQuad(item.WorldTransform, currentTexture, 1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);
    }

    void UIManager::RenderUIButton(entt::registry& registry, UIRenderItem& item)
    {
        entt::entity entity = item.Entity;
        auto& button = registry.get<UIButtonComponent>(entity);

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

        auto anchored = CalculateAnchoredTransform(registry, sliderComponent.Anchor, item);

        float rotation = transformComponent.GetLocalRotation().z;

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

        if ((sliderComponent.Selected && sliderComponent.HandleTexture) || sliderComponent.DisabledHandleTexture)
        {
            Renderer2D::DrawQuad(handleTransform,
                sliderComponent.Selected ? sliderComponent.HandleTexture : sliderComponent.DisabledHandleTexture,
                1.0f, glm::vec4(1.0f), Renderer2D::RenderMode::Screen, (uint32_t)entity);
        }
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

                    RectAnchor* parentAnchor = GetComponentAnchor(registry, item.Parent, parentItem.ComponentType);

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
                    RectAnchor* parentAnchor = GetComponentAnchor(registry, item.Parent, parentItem.ComponentType);

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

    UIManager::AnchoredTransform UIManager::CalculateAnchoredTransform(entt::registry& registry, const RectAnchor& anchor, UIRenderItem& item)
    {
        AnchoredTransform result;

        glm::vec2 referenceSize = CanvasReferenceSize;
        glm::vec2 parentSize = referenceSize;
        glm::vec2 parentPosition = glm::vec2(WindowSize.x / 2, WindowSize.y / 2);
        bool hasParent = false;

        if (item.Parent != entt::null)
        {
            hasParent = true;
            parentSize = GetParentSize(registry, item);

            if (registry.any_of<TransformComponent>(item.Parent) && !registry.any_of<UIComponent>(item.Parent))
            {
                auto& parentTransform = registry.get<TransformComponent>(item.Parent);

                bool parentIsUIElement = registry.any_of<UIImageComponent>(item.Parent) ||
                                       registry.any_of<UITextComponent>(item.Parent) ||
                                       registry.any_of<UIButtonComponent>(item.Parent) ||
                                       registry.any_of<UIToggleComponent>(item.Parent) ||
                                       registry.any_of<UISliderComponent>(item.Parent);

                if (parentIsUIElement)
                {
                    UIRenderItem* parentItem = nullptr;
                    for (auto& renderItem : s_SortedUIItems)
                    {
                        if (renderItem.Entity == item.Parent)
                        {
                            parentItem = &renderItem;
                            break;
                        }
                    }

                    if (parentItem)
                    {
                        RectAnchor* parentAnchor = GetComponentAnchor(registry, item.Parent, parentItem->ComponentType);
                        if (parentAnchor)
                        {
                            AnchoredTransform parentAnchored = CalculateAnchoredTransform(registry, *parentAnchor, *parentItem);
                            parentPosition = parentAnchored.Position;
                        }
                    }
                }
                else
                {
                    parentPosition = glm::vec2(parentTransform.GetLocalPosition());
                }
            }
            else
            {
                hasParent = false;
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
        for (auto& item : s_SortedUIItems)
        {
            if (item.Parent == parentEntity)
            {
                item.TransformDirty = true;
                item.ParentSizeDirty = true;
                MarkChildrenForUpdate(item.Entity);
            }
        }
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

    UIManager::UIRenderItem& UIManager::GetUIRenderItem(entt::entity entity)
    {
        for (auto& item : s_SortedUIItems)
        {
            if (item.Entity == entity)
                return item;
        }

        static UIRenderItem defaultItem;
        return defaultItem;
    }

    void UIManager::MarkDirty(entt::entity entity)
    {
        UIRenderItem& item = GetUIRenderItem(entity);
        item.TransformDirty = true;
        MarkChildrenForUpdate(entity);
    }

    void UIManager::UpdateUITranform(entt::registry& registry, UIRenderItem& item)
    {
        AnchoredTransform anchored;

        if (item.TransformDirty)
        {
            switch (item.ComponentType) {
            case UIComponentType::Image: {
                auto& imageComponent = registry.get<UIImageComponent>(item.Entity);
                anchored = CalculateAnchoredTransform(registry, imageComponent.Anchor, item);
                break;
            }
            case UIComponentType::Text: {
                auto& textComponent = registry.get<UITextComponent>(item.Entity);
                anchored = CalculateAnchoredTransform(registry, textComponent.Anchor, item);
                break;
            }
            case UIComponentType::Toggle: {
                auto& toggleComponent = registry.get<UIToggleComponent>(item.Entity);
                anchored = CalculateAnchoredTransform(registry, toggleComponent.Anchor, item);
                break;
            }
            case UIComponentType::Button: {
                auto& buttonComponent = registry.get<UIButtonComponent>(item.Entity);
                anchored = CalculateAnchoredTransform(registry, buttonComponent.Anchor, item);
                break;
            }
            case UIComponentType::Slider: {
                auto& sliderComponent = registry.get<UISliderComponent>(item.Entity);
                anchored = CalculateAnchoredTransform(registry, sliderComponent.Anchor, item);
                break;
            }
            }

            auto& transformComponent = registry.get<TransformComponent>(item.Entity);

            RectAnchor* anchor = GetComponentAnchor(registry, item.Entity, item.ComponentType);
            if (anchor)
            {
                glm::vec2 anchoredPos = anchor->GetAnchoredPosition(item.ParentSize);
                transformComponent.SetLocalPosition(glm::vec3(anchoredPos, 0.0f));
                transformComponent.SetLocalScale(glm::vec3(anchor->GetSize().x, anchor->GetSize().y, 1.0f));
            }

            float rotation = transformComponent.GetLocalRotation().z;

            item.WorldTransform = glm::mat4(1.0f);
            item.WorldTransform = glm::translate(item.WorldTransform, glm::vec3(anchored.Position, 0.0f));
            item.WorldTransform = glm::rotate(item.WorldTransform, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
            if (item.ComponentType == UIComponentType::Text)
                item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(1.0f, -1.0f, 1.0f));
            else
                item.WorldTransform = glm::scale(item.WorldTransform, glm::vec3(anchored.Size.x, -anchored.Size.y, 1.0f));

            item.TransformDirty = false;
        }
    }

    void UIManager::UpdateUITranformRecursive(entt::registry& registry, UIRenderItem& item)
    {
        UpdateUITranform(registry, item);

        for (auto& childItem : s_SortedUIItems)
        {
            if (childItem.Parent == item.Entity)
                UpdateUITranformRecursive(registry, childItem);
        }
    }

    RectAnchor* UIManager::GetComponentAnchor(entt::registry& registry, entt::entity entity, UIComponentType componentType)
    {
        switch (componentType) {
            case UIComponentType::Image:
                if (registry.any_of<UIImageComponent>(entity))
                    return &registry.get<UIImageComponent>(entity).Anchor;
                break;
            case UIComponentType::Text:
                if (registry.any_of<UITextComponent>(entity))
                    return &registry.get<UITextComponent>(entity).Anchor;
                break;
            case UIComponentType::Button:
                if (registry.any_of<UIButtonComponent>(entity))
                    return &registry.get<UIButtonComponent>(entity).Anchor;
                break;
            case UIComponentType::Toggle:
                if (registry.any_of<UIToggleComponent>(entity))
                    return &registry.get<UIToggleComponent>(entity).Anchor;
                break;
            case UIComponentType::Slider:
                if (registry.any_of<UISliderComponent>(entity))
                    return &registry.get<UISliderComponent>(entity).Anchor;
                break;
        }
        return nullptr;
    }

    void UIManager::ScaleUIElement(entt::registry& registry, entt::entity entity, const glm::vec2& scale)
    {
        UIRenderItem& item = GetUIRenderItem(entity);
        if (item.Entity == entt::null) return;

        RectAnchor* anchor = GetComponentAnchor(registry, entity, item.ComponentType);

        if (anchor)
        {
            glm::vec2 currentSize = anchor->GetSize();
            glm::vec2 newSize = currentSize * scale;
            anchor->SetSize(newSize, item.ParentSize);

            if (item.ComponentType == UIComponentType::Text && registry.any_of<UITextComponent>(entity))
            {
                auto& textComponent = registry.get<UITextComponent>(entity);
                textComponent.FontSize *= (scale.x + scale.y) / 2.0f;
            }

            item.TransformDirty = true;

            for (auto& childItem : s_SortedUIItems)
            {
                if (childItem.Parent == entity)
                {
                    ScaleUIElement(registry, childItem.Entity, scale);
                }
            }
        }
    }

    void UIManager::MoveUIElement(entt::registry& registry, entt::entity entity, const glm::vec2& offset)
    {
        UIRenderItem& item = GetUIRenderItem(entity);
        if (item.Entity == entt::null) return;

        RectAnchor* anchor = GetComponentAnchor(registry, entity, item.ComponentType);

        if (anchor)
        {
            glm::vec2 currentPos = anchor->GetAnchoredPosition(item.ParentSize);
            glm::vec2 newPos = currentPos + offset;
            anchor->SetAnchoredPosition(newPos, item.ParentSize);
            
            MarkDirty(entity);
        }
    }

    void UIManager::RotateUIElement(entt::registry& registry, entt::entity entity, float angle)
    {
        UIRenderItem& item = GetUIRenderItem(entity);
        if (item.Entity == entt::null) return;

        if (registry.any_of<TransformComponent>(entity))
        {
            auto& transform = registry.get<TransformComponent>(entity);

            glm::vec3 rotation = transform.GetLocalRotation();
            rotation.z += angle;
            transform.SetLocalRotation(rotation);

            item.TransformDirty = true;

            for (auto& childItem : s_SortedUIItems)
            {
                if (childItem.Parent == entity)
                {
                    RotateUIElement(registry, childItem.Entity, angle);
                }
            }
        }
    }

    void UIManager::ProcessPendingTransforms(entt::registry& registry)
    {
        for (auto& [entity, operations] : s_PendingTransforms)
        {
            for (auto& operation : operations)
            {
                switch (operation.type)
                {
                case TransformOperation::Type::Scale:
                    ScaleUIElement(registry, entity, operation.scale);
                    break;
                case TransformOperation::Type::Move:
                    MoveUIElement(registry, entity, operation.offset);
                    break;
                case TransformOperation::Type::Rotate:
                    RotateUIElement(registry, entity, operation.angle);
                    break;
                }
            }
        }
        s_PendingTransforms.clear();
    }

} // Coffee