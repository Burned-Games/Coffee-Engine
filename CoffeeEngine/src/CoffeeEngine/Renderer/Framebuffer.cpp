#include "Framebuffer.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/Texture.h"

#include <cstdint>
#include <glad/glad.h>
#include <tracy/Tracy.hpp>

#include <glm/vec4.hpp>

namespace Coffee {

    static const uint32_t s_MaxFramebufferSize = 8192;

    Framebuffer::Framebuffer(uint32_t width, uint32_t height)
        : m_Width(width), m_Height(height)
    {
        ZoneScoped;

        glCreateFramebuffers(1, &m_fboID);

        Invalidate();
    }

    Framebuffer::~Framebuffer()
    {
        glDeleteFramebuffers(1, &m_fboID);
    }

    void Framebuffer::Resize(uint32_t width, uint32_t height)
    {
        ZoneScoped;

        if(width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
        {
            COFFEE_CORE_WARN("Attempted to resize framebuffer to {0}, {1}", width, height);
            return;
        }

        m_Width = width;
        m_Height = height;

        /* for (auto& texture : m_ColorTextures)
        {
            texture->SetData(nullptr, m_Width * m_Height * 4);
        }

        m_DepthTexture->SetData(nullptr, m_Width * m_Height * 4); */

        Invalidate();
    }

    void Framebuffer::Invalidate()
    {
        ZoneScoped;

        if(m_fboID)
        {
            glDeleteFramebuffers(1, &m_fboID);

            glCreateFramebuffers(1, &m_fboID);
            glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);

            for (int i = 0; i < m_ColorTextures.size(); i++)
            {
                if(!m_ColorTextures[i]) continue;

                m_ColorTextures[i]->Resize(m_Width, m_Height);
                glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT0 + i, m_ColorTextures[i]->GetID(), 0);
            }

            /* if(m_ColorTextures.size() == 0)
            {
                glDrawBuffer(GL_NONE);
                glReadBuffer(GL_NONE);
            } */

            if(m_DepthTexture)
            {
                m_DepthTexture->Resize(m_Width, m_Height);
                glNamedFramebufferTexture(m_fboID, GL_DEPTH_STENCIL_ATTACHMENT, m_DepthTexture->GetID(), 0);
            }
        }
    }

    void Framebuffer::Bind()
    {
        ZoneScoped;

        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
        glViewport(0, 0, m_Width, m_Height);
    }

    void Framebuffer::UnBind()
    {
        ZoneScoped;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glm::vec4 Framebuffer::GetPixelColor(int x, int y, uint32_t attachmentIndex)
    {
        ZoneScoped;

        COFFEE_CORE_ASSERT(attachmentIndex < m_ColorTextures.size(), "Attachment index out of bounds");

        glBindFramebuffer(GL_FRAMEBUFFER, m_fboID);
        glReadBuffer(GL_COLOR_ATTACHMENT0 + attachmentIndex);

        glm::vec4 result;
        glReadPixels(x, y, 1, 1, GL_RGBA, GL_FLOAT, &result);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return result;
    }

    void Framebuffer::SetDrawBuffers(std::initializer_list<uint32_t> colorAttachments)
    {
        ZoneScoped;

        std::vector<uint32_t> drawBuffers;
        for (int i = 0; i < colorAttachments.size(); i++)
        {
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + colorAttachments.begin()[i]);
        }

        glNamedFramebufferDrawBuffers(m_fboID, drawBuffers.size(), drawBuffers.data());
    }

    void Framebuffer::AttachColorTexture(const Ref<Texture2D>& texture, uint32_t mipLevel)
    {
        ZoneScoped;

        m_ColorTextures.push_back(texture);
        glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT0 + m_ColorTextures.size() - 1, texture->GetID(), mipLevel);
    }

    void Framebuffer::AttachColorTexture(uint32_t attachmentIndex, const Ref<Texture2D>& texture, uint32_t mipLevel)
    {
        if(m_ColorTextures.size() <= attachmentIndex)
        {
            m_ColorTextures.resize(attachmentIndex + 1);
        }

        m_ColorTextures[attachmentIndex] = texture;

        glNamedFramebufferTexture(m_fboID, GL_COLOR_ATTACHMENT0 + attachmentIndex, texture->GetID(), mipLevel);
    }

    const Ref<Texture2D>& Framebuffer::GetColorAttachment(uint32_t attachmentIndex)
    {
        ZoneScoped;

        COFFEE_CORE_ASSERT(attachmentIndex < m_ColorTextures.size(), "Attachment index out of bounds");
        return m_ColorTextures[attachmentIndex];
    }

    void Framebuffer::AttachDepthTexture(const Ref<Texture2D>& texture)
    {
        ZoneScoped;

        m_DepthTexture = texture;
        glNamedFramebufferTexture(m_fboID, GL_DEPTH_STENCIL_ATTACHMENT, texture->GetID(), 0);
    }

    const Ref<Texture2D>& Framebuffer::GetDepthTexture()
    {
        ZoneScoped;

        COFFEE_CORE_ASSERT(m_DepthTexture, "Depth texture not set");
        return m_DepthTexture;
    }

    Ref<Framebuffer> Framebuffer::Create(uint32_t width, uint32_t height)
    {
        return CreateRef<Framebuffer>(width, height);
    }

}
