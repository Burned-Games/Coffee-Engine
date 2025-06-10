#include "RenderTarget.h"

namespace Coffee {

    RenderTarget::RenderTarget(const std::string& name, const glm::vec2& size)
        : m_Name(name), m_Size(size)
    {
    }

    void RenderTarget::Resize(uint32_t width, uint32_t height)
    {
        m_Size = {width, height};

        for (auto& [name, framebuffer] : m_Framebuffers)
        {
            framebuffer->Resize(width, height);
        }
    }

}