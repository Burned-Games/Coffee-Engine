#pragma once
#include <entt/entity/registry.hpp>
#include <unordered_map>
#include "UIAnchor.h"

namespace Coffee {

    class UIManager
    {
    public:
        enum class UIComponentType
        {
            Image,
            Text,
            Toggle,
            Button,
            Slider
        };

        /**
         * @brief Represents a renderable UI item.
         */
        struct UIRenderItem {
            entt::entity Entity; ///< The entity associated with the UI item.
            int Layer; ///< The rendering layer of the UI item.
            UIComponentType ComponentType; ///< The type of UI component.
            entt::entity Parent; ///< The parent entity in the hierarchy.
            entt::entity Next; ///< The next sibling entity in the hierarchy.
        };

        /**
         * @brief Represents the anchored transform of a UI element.
         */
        struct AnchoredTransform {
            glm::vec2 Position; ///< The position of the UI element.
            glm::vec2 Size; ///< The size of the UI element.
        };

        /**
         * @brief Updates the UI elements in the registry.
         * @param registry The entity registry containing UI elements.
         */
        static void UpdateUI(entt::registry& registry);

        /**
         * @brief Marks the UI elements for sorting.
         */
        static void MarkForSorting() { s_NeedsSorting = true; }

        /**
         * @brief Gets an anchor preset based on row and column indices.
         * @param row The row index (0=top, 1=middle, 2=bottom, 3=stretch).
         * @param column The column index (0=left, 1=center, 2=right, 3=stretch).
         * @return The corresponding AnchorPreset.
         */
        static AnchorPreset GetAnchorPreset(int row, int column);

        /**
         * @brief Gets the size of the parent element for a given entity.
         * @param registry The entity registry.
         * @param parentEntity The parent entity.
         * @return The size of the parent element as a glm::vec2.
         */
        static glm::vec2 GetParentSize(entt::registry& registry, entt::entity parentEntity);

    private:
        /**
         * @brief Checks if the transform of an entity has changed.
         * @param entity The entity to check.
         * @param newTransform The new transform to compare with the previous one.
         * @return True if the transform has changed, false otherwise.
         */
        static bool HasTransformChanged(entt::entity entity, const AnchoredTransform& newTransform);

        /**
         * @brief Sorts the UI elements in the registry based on their layers and hierarchy.
         * @param registry The entity registry containing UI elements.
         */
        static void SortUIElements(entt::registry& registry);

        /**
         * @brief Renders a UIImage component.
         * @param registry The entity registry.
         * @param entity The entity containing the UIImage component.
         */
        static void RenderUIImage(entt::registry& registry, entt::entity entity);

        /**
         * @brief Renders a UIText component.
         * @param registry The entity registry.
         * @param entity The entity containing the UIText component.
         */
        static void RenderUIText(entt::registry& registry, entt::entity entity);

        /**
         * @brief Renders a UIToggle component.
         * @param registry The entity registry.
         * @param entity The entity containing the UIToggle component.
         */
        static void RenderUIToggle(entt::registry& registry, entt::entity entity);

        /**
         * @brief Renders a UIButton component.
         * @param registry The entity registry.
         * @param entity The entity containing the UIButton component.
         */
        static void RenderUIButton(entt::registry& registry, entt::entity entity);

        /**
         * @brief Renders a UISlider component.
         * @param registry The entity registry.
         * @param entity The entity containing the UISlider component.
         */
        static void RenderUISlider(entt::registry& registry, entt::entity entity);

        /**
         * @brief Calculates the anchored transform for a UI element.
         * @param registry The entity registry.
         * @param entity The entity containing the UI element.
         * @param anchor The RectAnchor of the UI element.
         * @param windowSize The size of the window.
         * @return The calculated anchored transform.
         */
        static AnchoredTransform CalculateAnchoredTransform(entt::registry& registry, entt::entity entity, const RectAnchor& anchor, const glm::vec2& windowSize);
    public:
        static glm::vec2 WindowSize;

    private:
        static bool s_NeedsSorting;
        static std::vector<UIRenderItem> s_SortedUIItems;
        static std::unordered_map<entt::entity, AnchoredTransform> s_LastTransforms;
    };

} // Coffee