#include "RuntimeLayer.h"

#include "CoffeeEngine/Core/Application.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Embedded/FinalPassShader.inl"
#include "CoffeeEngine/Events/ApplicationEvent.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/Renderer.h"
#include "CoffeeEngine/Renderer/RendererAPI.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scripting/Lua/LuaBackend.h"
#include "CoffeeEngine/Scripting/ScriptManager.h"

#include <cstdint>
#include <filesystem>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <string>
#include <sys/types.h>
#include <tracy/Tracy.hpp>

#include <IconsLucide.h>

namespace Coffee {

    static Ref<Mesh> s_ScreenQuad;
    static Ref<Shader> s_FinalPassShader;

    RuntimeLayer::RuntimeLayer() : Layer("Runtime")
    {

    }

    void RuntimeLayer::OnAttach()
    {
        ZoneScoped;

        s_ScreenQuad = PrimitiveMesh::CreateQuad();
        s_FinalPassShader = CreateRef<Shader>("FinalPassShader", std::string(finalPassShaderSource));

        // Create texture from texture parameters
        TextureProperties textureProperties;
        textureProperties.Width = 1600;
        textureProperties.Height = 900;
        textureProperties.Format = ImageFormat::RGBA32F;
        textureProperties.srgb = false;
        textureProperties.GenerateMipmaps = false;
        textureProperties.Wrapping = TextureWrap::ClampToEdge;
        textureProperties.MinFilter = TextureFilter::Linear;
        textureProperties.MagFilter = TextureFilter::Linear;

        Ref<Texture2D> forwardColorTexture = Texture2D::Create(textureProperties);

        textureProperties.Format = ImageFormat::RGB8;
        Ref<Texture2D> forwardEntityIDTexture = Texture2D::Create(textureProperties);

        textureProperties.Format = ImageFormat::DEPTH24STENCIL8;
        Ref<Texture2D> forwardDepthTexture = Texture2D::Create(textureProperties);

        Ref<Framebuffer> forwardFramebuffer = Framebuffer::Create(1600, 900);
        forwardFramebuffer->AttachColorTexture(0, forwardColorTexture);
        forwardFramebuffer->AttachColorTexture(1, forwardEntityIDTexture);
        forwardFramebuffer->AttachDepthTexture(forwardDepthTexture);

        textureProperties.Format = ImageFormat::RGBA32F;
        Ref<Texture2D> postProcessingColorTextureA = Texture2D::Create(textureProperties);
        Ref<Texture2D> postProcessingColorTextureB = Texture2D::Create(textureProperties);

        Ref<Framebuffer> postProcessingFramebufferA = Framebuffer::Create(1600, 900);
        postProcessingFramebufferA->AttachColorTexture(0, postProcessingColorTextureA);

        Ref<Framebuffer> postProcessingFramebufferB = Framebuffer::Create(1600, 900);
        postProcessingFramebufferB->AttachColorTexture(0, postProcessingColorTextureB);

        m_ViewportRenderTarget = CreateRef<RenderTarget>("EditorViewport", glm::vec2(1600, 900));
        m_ViewportRenderTarget->AddFramebuffer("Forward", forwardFramebuffer);
        m_ViewportRenderTarget->AddFramebuffer("PostProcessingA", postProcessingFramebufferA);
        m_ViewportRenderTarget->AddFramebuffer("PostProcessingB", postProcessingFramebufferB);

        Renderer::AddRenderTarget(m_ViewportRenderTarget);

        ScriptManager::RegisterBackend(ScriptingLanguage::Lua, CreateRef<LuaBackend>());

        Project::Load(std::filesystem::current_path() / "gamedata" / "Default.TeaProject");
        Application::Get().GetWindow().SetTitle(Project::GetActive()->GetProjectName());
        Application::Get().GetWindow().SetIcon("icon.png");

        // Load the default scene from the project
        SceneManager::SetSceneState(SceneManager::SceneState::Play);
        SceneManager::ChangeScene(std::filesystem::current_path() / "gamedata" / "Default.TeaScene");

        m_ViewportSize = { 1600.0f, 900.0f };
    }

    void RuntimeLayer::OnUpdate(float dt)
    {
        ZoneScoped;

        Renderer::SetCurrentRenderTarget(m_ViewportRenderTarget.get());

        SceneManager::GetActiveScene()->OnUpdateRuntime(dt);

        Renderer::SetCurrentRenderTarget(nullptr);

        // Render the scene to backbuffer
        const Ref<Texture2D>& finalTexture = m_ViewportRenderTarget->GetFramebuffer("Forward")->GetColorAttachment(0);
        finalTexture->Bind(0);

        s_FinalPassShader->Bind();
        s_FinalPassShader->setInt("screenTexture", 0);

        RendererAPI::Clear();

        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

        s_FinalPassShader->Unbind();
    }

    void RuntimeLayer::OnEvent(Coffee::Event& event)
    {
        ZoneScoped;

        SceneManager::GetActiveScene()->OnEvent(event);

        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& event) {
            ResizeViewport((float)event.GetWidth(), (float)event.GetHeight());
            return false;
        });
    }

    void RuntimeLayer::OnDetach()
    {
        ZoneScoped;

        SceneManager::GetActiveScene()->OnExitRuntime();
    }

    void RuntimeLayer::ResizeViewport(float width, float height)
    {
        if((m_ViewportSize.x > 0.0f && m_ViewportSize.y > 0.0f) &&
           (width != m_ViewportSize.x || height != m_ViewportSize.y))
        {
            m_ViewportRenderTarget->Resize((uint32_t)width, (uint32_t)height);
        }

        m_ViewportSize = { width, height };
    }
}