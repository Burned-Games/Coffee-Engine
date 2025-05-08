#include "Renderer.h"
#include "Renderer3D.h"
#include "Renderer2D.h"
#include "CoffeeEngine/Renderer/RendererAPI.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"

#include <glm/matrix.hpp>
#include <tracy/Tracy.hpp>
#include <vector>

namespace Coffee {
    
    RendererData Renderer::s_RendererData;
    RendererSettings Renderer::s_RenderSettings;

    Ref<Mesh> Renderer::s_ScreenQuad;

    void Renderer::Init()
    {
        ZoneScoped;

        RendererAPI::Init();

        Renderer2D::Init();
        Renderer3D::Init();

        s_RendererData.CameraUniformBuffer = UniformBuffer::Create(sizeof(CameraData), 0);

        s_ScreenQuad = PrimitiveMesh::CreateQuad();
    }

    void Renderer::Render()
    {
        ZoneScoped;

        Renderer3D::ResetStats();

        for (auto& target : s_RendererData.RenderTargets)
        {
            CameraData cameraData;
            cameraData.view = glm::inverse(target.GetCameraTransform());
            cameraData.projection = target.GetCamera().GetProjection();
            cameraData.position = target.GetCameraTransform()[3];

            s_RendererData.CameraUniformBuffer->SetData(&cameraData, sizeof(CameraData));

            Renderer3D::ShadowPass(target);
            Renderer3D::ForwardPass(target);
            Renderer3D::SkyboxPass(target);
            Renderer3D::TransparentPass(target);

            if(s_RenderSettings.PostProcessing)
            {
                Renderer3D::PostProcessingPass(target);
            }

            // Think if this should be done before or after post processing
            RendererAPI::SetDepthMask(false);
            RendererAPI::SetFaceCulling(false);
            Renderer2D::WorldPass(target);
            RendererAPI::SetFaceCulling(true);
            RendererAPI::SetDepthMask(true);
            
            // TODO: Think if this should be done here or in the Renderer2D
            cameraData.projection = glm::ortho(0.0f, target.GetSize().x, target.GetSize().y, 0.0f, -1.0f, 1.0f);
            cameraData.view = glm::mat4(1.0f);
            s_RendererData.CameraUniformBuffer->SetData(&cameraData, sizeof(CameraData));
            
            RendererAPI::SetFaceCulling(false);
            RendererAPI::SetDepthMask(false);

            Renderer2D::ScreenPass(target);

            RendererAPI::SetDepthMask(true);
            RendererAPI::SetFaceCulling(true);
        }

        // TODO: Think if this should be done here or inside each target?
        Renderer3D::ResetCalls();
    }

    void Renderer::Shutdown()
    {
        Renderer3D::Shutdown();
    }

    const RenderTarget& Renderer::GetRenderTarget(const std::string& name)
    {
        ZoneScoped;

        for (auto& target : s_RendererData.RenderTargets)
        {
            if (target.GetName() == name)
            {
                return target;
            }
        }

        return s_RendererData.RenderTargets[0];
    }

    RenderTarget& Renderer::AddRenderTarget(const std::string& name, const glm::vec2& size, const std::vector<std::pair<std::string, std::initializer_list<Attachment>>>& framebufferAttachments)
    {
        ZoneScoped;

        s_RendererData.RenderTargets.push_back(RenderTarget(name, size, framebufferAttachments));
        return s_RendererData.RenderTargets.back();
    }
}