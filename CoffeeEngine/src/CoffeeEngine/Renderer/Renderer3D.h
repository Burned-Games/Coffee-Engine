#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Renderer/EditorCamera.h"
#include "CoffeeEngine/Renderer/Framebuffer.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/RenderTarget.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Renderer/UniformBuffer.h"
#include "CoffeeEngine/Renderer/VertexArray.h"
#include "CoffeeEngine/Scene/Components.h"
#include <glm/fwd.hpp>

namespace Coffee {

    /**
     * @defgroup renderer Renderer
     * @brief Renderer components of the CoffeeEngine.
     * @{
     */

    struct RenderCommand
    {
        glm::mat4 transform = glm::mat4(1.0f);
        Ref<Mesh> mesh;
        Ref<Material> material;
        uint32_t entityID = 4294967295;
        AnimatorComponent* animator;
    };

    /**
     * @brief Structure containing renderer data.
     */
    struct Renderer3DData
    {
        /**
         * @brief Structure containing render data.
         */
        struct SceneRenderData
        {
            LightComponent lights[32]; ///< Array of light components.
            int lightCount = 0; ///< Number of lights.
        };

        SceneRenderData RenderData; ///< Render data.

        Ref<UniformBuffer> SceneRenderDataUniformBuffer; ///< Uniform buffer for render data.

        Ref<Material> DefaultMaterial; ///< Default material.
        Ref<Mesh> MissingMesh; ///< Missing mesh.
        Ref<Cubemap> DefaultSkybox; ///< Default skybox.

        Ref<Texture2D> BRDFLUT; ///< BRDF LUT texture.

        Ref<Framebuffer> ShadowMapFramebuffer;
        static constexpr int MAX_DIRECTIONAL_SHADOWS = 4;
        Ref<Texture2D> DirectionalShadowMapTextures[4];

        Ref<Cubemap> EnvironmentMap;

        std::vector<RenderCommand> opaqueRenderQueue; ///< Opaque render queue.
        std::vector<RenderCommand> transparentRenderQueue; ///< Transparent render queue.
    };

    /**
     * @brief Structure containing renderer statistics.
     */
    struct Renderer3DStats
    {
        uint32_t DrawCalls = 0; ///< Number of draw calls.
        uint32_t VertexCount = 0; ///< Number of vertices.
        uint32_t IndexCount = 0; ///< Number of indices.

        void Reset()
        {
            DrawCalls = 0;
            VertexCount = 0;
            IndexCount = 0;
        }
    };

    /**
     * @brief Structure containing render settings.
     */
    struct Renderer3DSettings
    {
        bool SSAO = false; ///< Enable or disable SSAO.

        bool DepthFog = false; ///< Enable or disable depth fog.
        glm::vec3 FogColor = {0.5f, 0.5f, 0.5f}; ///< Fog color.
        float FogDensity = 0.1f; ///< Fog density.
        float FogHeight = 0.0f; ///< Fog height.
        float FogHeightDensity = 0.0f; ///< Fog height density.

        bool Bloom = false; ///< Enable or disable bloom.
        float BloomThreshold = 1.0f; ///< Bloom threshold.
        float BloomIntensity = 1.0f; ///< Bloom intensity.
        float BloomRadius = 1.0f; ///< Bloom radius.
        float BloomScale = 1.0f; ///< Bloom scale.

        bool FXAA = true; ///< Enable or disable FXAA.

        float Exposure = 1.0f; ///< Exposure value.
        float EnvironmentExposure = 1.0f; ///< Environment exposure value.

        // REMOVE: This is for the first release of the engine it should be handled differently
        bool showNormals = false;
    };

    /**
     * @brief Class representing the 3D renderer.
     */
    class Renderer3D
    {
    public:
        /**
         * @brief Initializes the renderer.
         */
        static void Init();

        /**
         * @brief Shuts down the renderer.
         */
        static void Shutdown();

        static void Submit(const RenderCommand& command);

        static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f), uint32_t entityID = 4294967295);

        /**
         * @brief Submits a light component.
         * @param light The light component.
         */

         //Todo change this to a light class and not a component
        static void Submit(const LightComponent& light);

        static void SetEnvironmentMap(const Ref<Cubemap>& environmentMap) { s_RendererData.EnvironmentMap = environmentMap; }
        
        static void DepthPrePass(const RenderTarget& target);
        //static void SSAOPass(const RenderTarget& target);
        static void ShadowPass(const RenderTarget& target);
        static void ForwardPass(const RenderTarget& target);
        static void SkyboxPass(const RenderTarget& target);
        static void TransparentPass(const RenderTarget& target);
        static void PostProcessingPass(const RenderTarget& target);

        /**
         * @brief Gets the renderer data.
         * @return A reference to the renderer data.
         */
        static const Renderer3DData& GetData() { return s_RendererData; }

        /**
         * @brief Gets the renderer statistics.
         * @return A reference to the renderer statistics.
         */
        static const Renderer3DStats& GetStats() { return s_Stats; }
        static void ResetStats() { s_Stats.Reset(); }

        /**
         * @brief Gets the render settings.
         * @return A reference to the render settings.
         */
        static Renderer3DSettings& GetRenderSettings() { return s_RenderSettings; }
        
        // TODO: Think better name for this
        /*
            - Reset Lights Count
            - Reset Render Queue
        */
        static void ResetCalls();
    
    private:
        static void GenerateBRDFLUT();

    private:
        static Renderer3DData s_RendererData; ///< Renderer data.
        static Renderer3DStats s_Stats; ///< Renderer statistics.
        static Renderer3DSettings s_RenderSettings; ///< Render settings.

        static Ref<Mesh> s_ScreenQuad; ///< Screen quad mesh.
        static Ref<Mesh> s_CubeMesh; ///< Cube mesh.

        static Ref<Shader> s_FogShader; ///< Fog shader.
        static Ref<Shader> s_ToneMappingShader; ///< Tone mapping shader.
        static Ref<Shader> s_FXAAShader; ///< Fast Approximate AntiAliasing shader
        static Ref<Shader> s_FinalPassShader; ///< Final pass shader.
        static Ref<Shader> s_SkyboxShader; ///< Skybox shader.
        static Ref<Shader> depthShader; ///< Depth shader.
        static Ref<Shader> brdfShader; ///< BRDF shader.
    };

    /** @} */
}