#pragma once

#include "CoffeeEngine/Core/Base.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <vector>

/**
 * @brief Structure representing a point in a gradient.
 */
struct GradientPoint
{
    float position; // Range 0.0 - 1.0 (position in the gradient bar)
    ImVec4 color;   // Color in RGBA

    /**
     * @brief Serializes a GradientPoint object.
     * @tparam Archive The type of the archive.
     * @param archive The archive to serialize to.
     */
    template <class Archive> void serialize(Archive& archive, std::uint32_t const version) { archive(position, color); }
};

/**
 * @brief Structure representing a point in a curve.
 */
struct CurvePoint
{
    float time;  // Range -1.0 - 1.0
    float value; // Size scale

    /**
     * @brief Serializes a CurvePoint object.
     * @tparam Archive The type of the archive.
     * @param archive The archive to serialize to.
     */
    template <class Archive> void serialize(Archive& archive, std::uint32_t const version) { archive(time, value); }
};

/**
 * @brief Static class for gradient editing.
 */
class GradientEditor
{
  public:
    /**
     * @brief Draws the gradient preview.
     * @param points Gradient points.
     * @param canvas_pos Canvas position.
     * @param canvas_size Canvas size.
     */
    static void DrawGradientBar(const std::vector<GradientPoint>& points, ImVec2 canvas_pos, ImVec2 canvas_size);

    /**
     * @brief Handles gradient control points.
     * @param points Gradient points.
     * @param canvas_pos Canvas position.
     * @param canvas_size Canvas size.
     */
    static bool EditGradientPoints(std::vector<GradientPoint>& points, ImVec2 canvas_pos, ImVec2 canvas_size);

    /**
     * @brief Displays the gradient editor in ImGui.
     * @param points Gradient points.
     */
    static bool ShowGradientEditor(std::vector<GradientPoint>& points);

    /**
     * @brief Gets the color at a specific position in the gradient.
     * @param t Position in the gradient (0.0 to 1.0).
     * @param points Gradient points.
     * @return Interpolated color at the given position.
     */
    static ImVec4 GetGradientValue(float t, const std::vector<GradientPoint>& points);
};

/**
 * @brief Class for curve editing.
 */
class CurveEditor
{
  public:
    /**
     * @brief Draws the curve in an ImGui window.
     * @param label Curve label.
     * @param points Curve points.
     * @param graph_size Graph size.
     */
    static bool DrawCurve(const char* label, std::vector<CurvePoint>& points, ImVec2 graph_size = ImVec2(200, 50));

    /**
     * @brief Gets the curve value at a specific time.
     * @param time Time at which to evaluate the curve.
     * @param points Curve points.
     * @return Curve value at the specified time.
     */
    static float GetCurveValue(float time, const std::vector<CurvePoint>& points);


    /**
     * @brief Gets the scale curve value at a specific range.
     * @param curveValue Value of the curve [0,1] for the scale.
     * @param min Min value.
     * @param max Max value.
     * @return Return value scaled
     */
    static float ScaleCurveValue(float curveValue, float min, float max);
};

/**
 * @brief Serializes an ImVec4 object.
 * @tparam Archive The type of the archive.
 * @param archive The archive to serialize to.
 * @param vec The ImVec4 object to serialize.
 */
template <class Archive> void serialize(Archive& archive, ImVec4& vec)
{
    archive(vec.x, vec.y, vec.z, vec.w); // Serializes the x, y, z, w components of ImVec4
}