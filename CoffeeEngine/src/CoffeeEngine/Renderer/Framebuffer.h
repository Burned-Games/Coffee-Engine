#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include <cstdint>
#include <initializer_list>
#include <sys/types.h>
#include <vector>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief Class representing a framebuffer.
     */
    class Framebuffer
    {
    public:
        /**
         * @brief Constructs a Framebuffer with the specified width, height, and attachments.
         * @param width The width of the framebuffer.
         * @param height The height of the framebuffer.
         * @param attachments The list of image formats for the attachments.
         */
        Framebuffer(uint32_t width, uint32_t height);

        /**
         * @brief Destroys the Framebuffer.
         */
        ~Framebuffer();

        /**
         * @brief Invalidates the framebuffer, forcing it to be recreated.
         */
        void Invalidate();

        /**
         * @brief Binds the framebuffer.
         */
        void Bind();

        /**
         * @brief Unbinds the framebuffer.
         */
        void UnBind();

        void AttachColorTexture(const Ref<Texture2D>& texture, uint32_t mipLevel = 0);
        void AttachColorTexture(uint32_t attachmentIndex, const Ref<Texture2D>& texture, uint32_t mipLevel = 0);
        
        const Ref<Texture2D>& GetColorAttachment(uint32_t attachmentIndex);
        
        void AttachDepthTexture(const Ref<Texture2D>& texture);

        const Ref<Texture2D>& GetDepthTexture();

        glm::vec4 GetPixelColor(int x, int y, uint32_t attachmentIndex = 0);

        /**
         * @brief Sets the draw buffers for the framebuffer.
         * @param colorAttachments The list of color attachment indices.
         */
        void SetDrawBuffers(std::initializer_list<uint32_t> indices);

        /**
         * @brief Resizes the framebuffer.
         * @param width The new width of the framebuffer.
         * @param height The new height of the framebuffer.
         */
        void Resize(uint32_t width, uint32_t height);

        /**
         * @brief Gets the width of the framebuffer.
         * @return The width of the framebuffer.
         */
        const uint32_t GetWidth() const { return m_Width; }

        /**
         * @brief Gets the height of the framebuffer.
         * @return The height of the framebuffer.
         */
        const uint32_t GetHeight() const { return m_Height; }

        /**
         * @brief Gets the depth texture.
         * @return A reference to the depth texture.
         */
        const Ref<Texture2D>& GetDepthTexture() const { return m_DepthTexture; }

        /**
         * @brief Creates a framebuffer with the specified width, height, and attachments.
         * @param width The width of the framebuffer.
         * @param height The height of the framebuffer.
         * @param attachments The list of image formats for the attachments.
         * @return A reference to the created framebuffer.
         */
        static Ref<Framebuffer> Create(uint32_t width, uint32_t height);

    private:
        uint32_t m_fboID; ///< The ID of the framebuffer object.

        uint32_t m_Width; ///< The width of the framebuffer.
        uint32_t m_Height; ///< The height of the framebuffer.

        std::vector<Ref<Texture2D>> m_ColorTextures; ///< The list of color textures.
        Ref<Texture2D> m_DepthTexture; ///< The depth texture.
    };

    /** @} */
}