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
            Empty,
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
            glm::mat4 WorldTransform; ///< Cached world transform matrix
            bool TransformDirty = true; ///< Flag to indicate if transform needs updating
            glm::vec2 ParentSize; ///< Cached size of the parent element
            bool ParentSizeDirty = true; ///< Flag to indicate if parent size needs updating
        };

        /**
         * @brief Represents the anchored transform of a UI element.
         */
        struct AnchoredTransform {
            glm::vec2 Position; ///< The position of the UI element.
            glm::vec2 Size; ///< The size of the UI element.
        };

        struct TransformOperation {
            enum class Type {
                None,
                Scale,
                Move,
                Rotate
            };

            Type type = Type::None;

            union {
                glm::vec2 scale;
                glm::vec2 offset;
                float angle;
            };

            static TransformOperation CreateScale(const glm::vec2& scale) {
                TransformOperation op;
                op.type = Type::Scale;
                op.scale = scale;
                return op;
            }

            static TransformOperation CreateMove(const glm::vec2& offset) {
                TransformOperation op;
                op.type = Type::Move;
                op.offset = offset;
                return op;
            }

            static TransformOperation CreateRotate(float angle) {
                TransformOperation op;
                op.type = Type::Rotate;
                op.angle = angle;
                return op;
            }
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
         * @return The size of the parent element as a glm::vec2.
         */
        static glm::vec2 GetParentSize(entt::registry& registry, UIRenderItem& item);

        /**
        * @brief Sets the reference canvas size that the UI was designed for.
        * @param referenceSize The reference size the UI was designed for.
        */
        static void SetReferenceCanvasSize(const glm::vec2& referenceSize);

        /**
         * @brief Calculates the UI scale factor based on current window size and reference size.
         */
        static void CalculateUIScaleFactor();

        /**
         * @brief Converts a size from the reference canvas space to the current window space.
         * @param size Size in reference space.
         * @return Size in current window space.
         */
        static glm::vec2 ScaleSize(const glm::vec2& size);

        /**
         * @brief Converts a position from the reference canvas space to the current window space.
         * @param position Position in reference space.
         * @return Position in current window space.
         */
        static glm::vec2 ScalePosition(const glm::vec2& position);

        /**
         * @brief Gets the UIRenderItem associated with a given entity.
         * @param entity The entity to get the UIRenderItem for.
         * @return A reference to the UIRenderItem associated with the entity.
         */
        static UIRenderItem& GetUIRenderItem(entt::entity entity);

        /**
         * @brief Marks a UI element as dirty, indicating that it needs to be updated.
         * @param entity The entity to mark as dirty.
         */
        static void MarkDirty(entt::entity entity);

        /**
         * @brief Queues a scale transformation to be applied during the next update.
         * @param entity The entity to scale.
         * @param scale The scale factor to apply.
         */
        static void MarkToTransform(entt::entity entity, const glm::vec2& scale) {
            s_PendingTransforms[entity].push_back(TransformOperation::CreateScale(scale));
        }

        /**
         * @brief Queues a move transformation to be applied during the next update.
         * @param entity The entity to move.
         * @param offset The amount to move the entity by.
         * @param isMove Parameter to differentiate from the scale overload (should always be true).
         */
        static void MarkToTransform(entt::entity entity, const glm::vec2& offset, bool isMove) {
            s_PendingTransforms[entity].push_back(TransformOperation::CreateMove(offset));
        }

        /**
         * @brief Queues a rotation transformation to be applied during the next update.
         * @param entity The entity to rotate.
         * @param angle The rotation angle in degrees.
         * @param absolute If true, sets the absolute rotation; if false, adds to the current rotation.
         */
        static void MarkToTransform(entt::entity entity, float angle, bool absolute = false) {
            s_PendingTransforms[entity].push_back(TransformOperation::CreateRotate(angle));
        }

    private:
        /**
         * @brief Marks all child entities of a given parent entity for update.
         * @param parentEntity The parent entity whose children need to be marked.
         */
        static void MarkChildrenForUpdate(entt::entity parentEntity);

        /**
         * @brief Sorts the UI elements in the registry based on their layers and hierarchy.
         * @param registry The entity registry containing UI elements.
         */
        static void SortUIElements(entt::registry& registry);

        /**
         * @brief Renders a UIImage component.
         * @param registry The entity registry.
         * @param item Reference to the corresponding UIRenderItem.
         */
        static void RenderUIImage(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Renders a UIText component.
         * @param registry The entity registry.
         * @param item Reference to the corresponding UIRenderItem.
         */
        static void RenderUIText(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Renders a UIToggle component.
         * @param registry The entity registry.
         * @param item Reference to the corresponding UIRenderItem.
         */
        static void RenderUIToggle(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Renders a UIButton component.
         * @param registry The entity registry.
         * @param item Reference to the corresponding UIRenderItem.
         */
        static void RenderUIButton(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Renders a UISlider component.
         * @param registry The entity registry.
         * @param item Reference to the corresponding UIRenderItem.
         */
        static void RenderUISlider(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Calculates the anchored transform for a UI element.
         * @param registry The entity registry.
         * @param entity The entity containing the UI element.
         * @param anchor The RectAnchor of the UI element.
         * @return The calculated anchored transform.
         */
        static AnchoredTransform CalculateAnchoredTransform(entt::registry& registry, const RectAnchor& anchor, UIRenderItem& item);

        /**
         * @brief Updates the transform of a UI element.
         * @param registry The entity registry.
         * @param item Reference to the UIRenderItem to update.
         */
        static void UpdateUITranform(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Recursively updates the transform of a UI element and its children.
         * @param registry The entity registry.
         * @param item Reference to the UIRenderItem to update.
         */
        static void UpdateUITranformRecursive(entt::registry& registry, UIRenderItem& item);

        /**
         * @brief Processes pending transformations for UI elements.
         * @param registry The entity registry.
         */
        static void ProcessPendingTransforms(entt::registry& registry);

        /**
         * @brief Scales a UI element and all of its children by adjusting their sizes.
         * @param registry The entity registry containing UI elements.
         * @param entity The entity containing the UI element.
         * @param scale The scale factor to apply.
         */
        static void ScaleUIElement(entt::registry& registry, entt::entity entity, const glm::vec2& scale);

        /**
         * @brief Moves a UI element by adjusting its position.
         * @param registry The entity registry containing UI elements.
         * @param entity The entity containing the UI element.
         * @param offset The amount to move the element by.
         */
        static void MoveUIElement(entt::registry& registry, entt::entity entity, const glm::vec2& offset);

        /**
         * @brief Rotates a UI element and all of its children.
         * @param registry The entity registry containing UI elements.
         * @param entity The entity containing the UI element.
         * @param angle The rotation angle in degrees.
         */
        static void RotateUIElement(entt::registry& registry, entt::entity entity, float angle);

        /**
         * @brief Gets the anchor pointer from a UI component based on its type.
         * @param registry The entity registry containing UI elements.
         * @param entity The entity containing the UI element.
         * @param componentType The type of UI component.
         * @return A pointer to the RectAnchor of the component, or nullptr if not found.
         */
        static RectAnchor* GetComponentAnchor(entt::registry& registry, entt::entity entity, UIComponentType componentType);

    public:
        static glm::vec2 WindowSize;

    private:
        static bool s_NeedsSorting;
        static std::vector<UIRenderItem> s_SortedUIItems;
        static std::unordered_map<entt::entity, AnchoredTransform> s_LastTransforms;

        static glm::vec2 CanvasReferenceSize;
        static float UIScale;
        static glm::vec2 m_lastWindowSize;

        static std::unordered_map<entt::entity, std::vector<TransformOperation>> s_PendingTransforms;
    };

} // Coffee