#pragma once

#include <cereal/cereal.hpp>
#include <glm/glm.hpp>

namespace Coffee {

    /**
     * @brief Represents a preset for anchoring UI elements along the X-axis.
     */
    enum class AnchorPresetX
    {
        Left,   ///< Anchor to the left side.
        Center, ///< Anchor to the center.
        Right,  ///< Anchor to the right side.
        Stretch ///< Stretch across the X-axis.
    };

    /**
     * @brief Represents a preset for anchoring UI elements along the Y-axis.
     */
    enum class AnchorPresetY
    {
        Top,    ///< Anchor to the top side.
        Middle, ///< Anchor to the middle.
        Bottom, ///< Anchor to the bottom side.
        Stretch ///< Stretch across the Y-axis.
    };

    /**
     * @brief Represents a combination of X and Y anchor presets.
     */
    struct AnchorPreset
    {
        AnchorPresetX X; ///< The X-axis anchor preset.
        AnchorPresetY Y; ///< The Y-axis anchor preset.

        AnchorPreset() : X(AnchorPresetX::Center), Y(AnchorPresetY::Middle) {}

        AnchorPreset(AnchorPresetX x, AnchorPresetY y) : X(x), Y(y) {}
    };

    /**
     * @brief Represents the anchor and offset properties for a UI element.
     */
    struct RectAnchor
    {
        glm::vec2 AnchorMin = { 0.5f, 0.5f }; ///< Minimum anchor point.
        glm::vec2 AnchorMax = { 0.5f, 0.5f }; ///< Maximum anchor point.

        glm::vec2 OffsetMin = { -50.0f, -50.0f }; ///< Minimum offset from the anchor.
        glm::vec2 OffsetMax = { 50.0f, 50.0f }; ///< Maximum offset from the anchor.

        /**
         * @brief Sets the anchor preset for the RectAnchor.
         * @param preset The anchor preset to apply.
         * @param currentRect The current rectangle dimensions.
         * @param parentSize The size of the parent element.
         * @param preservePosition Whether to preserve the current position.
         */
        void SetAnchorPreset(AnchorPreset preset, const glm::vec4& currentRect, const glm::vec2& parentSize, bool preservePosition);

        /**
         * @brief Calculates the rectangle dimensions based on the parent size.
         * @param parentSize The size of the parent element.
         * @return The calculated rectangle dimensions.
         */
        glm::vec4 CalculateRect(const glm::vec2& parentSize) const;

        /**
         * @brief Calculates the transform data (position and size) based on the parent size.
         * @param parentSize The size of the parent element.
         * @param position The calculated position.
         * @param size The calculated size.
         */
        void CalculateTransformData(const glm::vec2& parentSize, glm::vec2& position, glm::vec2& size) const;

        /**
         * @brief Gets the position and size of the RectAnchor based on the parent size.
         * @param parentSize The size of the parent element.
         * @return The position and size as a glm::vec4.
         */
        glm::vec4 GetPositionAndSize(const glm::vec2& parentSize) const;

        /**
         * @brief Gets the anchored position based on the parent size.
         * @param parentSize The size of the parent element.
         * @return The anchored position as a glm::vec2.
         */
        glm::vec2 GetAnchoredPosition(const glm::vec2& parentSize) const;

        /**
         * @brief Sets the anchored position based on the parent size.
         * @param position The new position to set.
         * @param parentSize The size of the parent element.
         */
        void SetAnchoredPosition(const glm::vec2& position, const glm::vec2& parentSize);

        /**
         * @brief Gets the size of the RectAnchor.
         * @return The size as a glm::vec2.
         */
        glm::vec2 GetSize() const;

        /**
         * @brief Sets the size of the RectAnchor.
         * @param size The new size to set.
         * @param parentSize The size of the parent element.
         */
        void SetSize(const glm::vec2& size, const glm::vec2& parentSize);

        template<class Archive> void save(Archive& archive, std::uint32_t const version) const
        {
            archive(cereal::make_nvp("AnchorMin", AnchorMin),
                    cereal::make_nvp("AnchorMax", AnchorMax),
                    cereal::make_nvp("OffsetMin", OffsetMin),
                    cereal::make_nvp("OffsetMax", OffsetMax));
        }

        template<class Archive> void load(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::make_nvp("AnchorMin", AnchorMin),
                    cereal::make_nvp("AnchorMax", AnchorMax),
                    cereal::make_nvp("OffsetMin", OffsetMin),
                    cereal::make_nvp("OffsetMax", OffsetMax));
        }
    };

} // Coffee