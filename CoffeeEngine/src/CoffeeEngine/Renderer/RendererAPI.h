#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/VertexArray.h"

#include <cstdint>
#include <glm/glm.hpp>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    enum ClearFlags : uint32_t
    {
        Color = 1 << 0,
        Depth = 1 << 1,
        Stencil = 1 << 2,
        ColorDepth = Color | Depth
    };

    enum class DepthFunc
    {
        Never,
        Less,
        Equal,
        LessEqual,
        Greater,
        NotEqual,
        GreaterEqual,
        Always

    };

    enum class BlendFunc
    {
        Zero = 0,
        One = 1,
        SrcColor = 0x0300,
        OneMinusSrcColor = 0x0301,
        SrcAlpha = 0x0302,
        OneMinusSrcAlpha = 0x0303,
        DstColor = 0x0306,
        OneMinusDstColor = 0x0307,
        DstAlpha = 0x0304,
        OneMinusDstAlpha = 0x0305
    };

    enum class BlendEquation
    {
        Add = 0x8006,
        Subtract = 0x800A,
        ReverseSubtract = 0x800B,
        Min = 0x8007,
        Max = 0x8008
    };
    
    enum class CullFace 
    {
    Front = 0,
    Back = 1,
    FrontAndBack = 2
    };

    enum class PolygonMode
    {
        Fill = 0,
        Line = 1,
        Point = 2
    };

    /**
     * @brief Class representing the Renderer API.
     */
    class RendererAPI {
    public:
        /**
         * @brief Initializes the Renderer API.
         */
        static void Init();

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);

        /**
         * @brief Sets the clear color for the renderer.
         * @param color The clear color as a glm::vec4.
         */
        static void SetClearColor(const glm::vec4& color);

        /**
         * @brief Clears the current buffer.
         */
        static void Clear(uint32_t clearFlags = (uint32_t)ClearFlags::Color | (uint32_t)ClearFlags::Depth);

        static void SetColorMask(bool red, bool green, bool blue, bool alpha);

        /**
         * @brief Enables or disables the depth mask.
         * @param enabled True to enable the depth mask, false to disable it.
         */
        static void SetDepthMask(bool enabled);

        static void SetDepthFunc(DepthFunc func);

        static void SetBlend(bool enabled);
        static void SetBlendFunc(BlendFunc src, BlendFunc dst);
        static void SetBlendEquation(BlendEquation equation);

        static void SetFaceCulling(bool enabled);

        static void SetCullFace(CullFace face);

        static void SetPolygonMode(PolygonMode mode);

        /**
         * @brief Draws the indexed vertices from the specified vertex array.
         * @param vertexArray The vertex array containing the vertices to draw.
         */
        static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0);

        /**
         * @brief Draws lines from the specified vertex array.
         * @param vertexArray The vertex array containing the vertices to draw.
         * @param vertexCount The number of vertices to draw.
         * @param lineWidth The width of the lines.
         */
        static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount, float lineWidth = 1.0f);

        /**
         * @brief Creates a new Renderer API instance.
         * @return A scope pointer to the created Renderer API instance.
         */
        static Scope<RendererAPI> Create();
    private:
        static Scope<RendererAPI> s_RendererAPI; ///< The Renderer API instance.
    };

    /** @} */
}