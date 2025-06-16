#pragma once

#include "CoffeeEngine/Core/Assert.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/RenderTarget.h"
#include "CoffeeEngine/Renderer/UniformBuffer.h"
#include <glm/fwd.hpp>
#include <unordered_map>
#include <vector>
namespace Coffee {

    struct CameraData
    {
        glm::mat4 projection = glm::mat4(1.0f); ///< The projection matrix.
        glm::mat4 view = glm::mat4(1.0f); ///< The view matrix.
        glm::vec3 position = {0.0, 0.0, 0.0}; ///< The position of the camera.
    };

    struct RendererData
    {
        std::unordered_map<std::string, Ref<RenderTarget>> RenderTargets;
        RenderTarget* CurrentRenderTarget = nullptr;

        Ref<UniformBuffer> CameraUniformBuffer; ///< Uniform buffer for camera data.
        CameraData cameraData; ///< Camera data.
    };

    struct RendererStats
    {
    };

    struct RendererSettings
    {
        bool PostProcessing = true; ///< Enable or disable post-processing.
    };

    class Renderer
    {
    public:
        
        static void Init();
        static void Render();
        static void Shutdown();

        static void AddRenderTarget(const Ref<RenderTarget>& renderTarget);
        static void RemoveRenderTarget(const std::string& name);
        static Ref<RenderTarget> GetRenderTarget(const std::string& name);

        static void SetCurrentRenderTarget(const std::string& name)
        { 
            s_RendererData.CurrentRenderTarget = GetRenderTarget(name).get();
            COFFEE_ASSERT(s_RendererData.CurrentRenderTarget && "Render target not found");
        }

        // This is more dangerous bc the renderTarget can not be in the map but is convenient
        static void SetCurrentRenderTarget(RenderTarget* renderTarget)
        {
            s_RendererData.CurrentRenderTarget = renderTarget;
            COFFEE_ASSERT(s_RendererData.CurrentRenderTarget && "Render target not found");
        }

        static RenderTarget* GetCurrentRenderTarget() { return s_RendererData.CurrentRenderTarget; }

        static RendererSettings& GetRenderSettings() { return s_RenderSettings; }

    private:
        static RendererData s_RendererData; ///< Renderer data.
        static RendererSettings s_RenderSettings; ///< Render settings.
        static Ref<Mesh> s_ScreenQuad; ///< Screen quad mesh.
    };

}