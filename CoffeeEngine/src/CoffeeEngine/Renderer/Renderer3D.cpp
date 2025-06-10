#include "Renderer3D.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Renderer/Framebuffer.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/RendererAPI.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Renderer/UniformBuffer.h"

#include "CoffeeEngine/Embedded/ToneMappingShader.inl"
#include "CoffeeEngine/Embedded/FinalPassShader.inl"
#include "CoffeeEngine/Embedded/MissingShader.inl"
#include "CoffeeEngine/Embedded/SimpleDepthShader.inl"
#include "CoffeeEngine/Embedded/BRDFLUTShader.inl"

#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    Renderer3DData Renderer3D::s_RendererData;
    Renderer3DStats Renderer3D::s_Stats;
    Renderer3DSettings Renderer3D::s_RenderSettings;

    Ref<Mesh> Renderer3D::s_ScreenQuad;
    Ref<Mesh> Renderer3D::s_CubeMesh;

    Ref<Shader> Renderer3D::s_FogShader;
    Ref<Shader> Renderer3D::s_ToneMappingShader;
    Ref<Shader> Renderer3D::s_FXAAShader;
    Ref<Shader> Renderer3D::s_FinalPassShader;
    Ref<Shader> Renderer3D::s_SkyboxShader;
    Ref<Shader> Renderer3D::depthShader;
    Ref<Shader> Renderer3D::brdfShader;
    Ref<Shader> Renderer3D::s_BloomShader;

    Ref<Framebuffer> Renderer3D::s_BloomFramebuffer;
    Ref<Texture2D> Renderer3D::s_BloomDownsampleTexture;
    Ref<Texture2D> Renderer3D::s_BloomUpsampleTexture;

    void Renderer3D::Init()
    {
        ZoneScoped;
        
        s_RendererData.DefaultSkybox = Cubemap::Load("assets/textures/StandardCubeMap.hdr");
        s_CubeMesh = PrimitiveMesh::CreateCube({-1.0f, -1.0f, -1.0f});
        s_SkyboxShader = CreateRef<Shader>("assets/shaders/SkyboxShader.glsl");

        depthShader = CreateRef<Shader>("DepthShader", std::string(simpleDepthShaderSource));

        brdfShader = CreateRef<Shader>("BRDFLUTShader", std::string(BRDFLUTSource));

        // Shadow map
        TextureProperties shadowMapProperties;
        shadowMapProperties.srgb = false;
        shadowMapProperties.GenerateMipmaps = false;
        shadowMapProperties.Format = ImageFormat::DEPTH24STENCIL8;
        shadowMapProperties.Width = 4096;
        shadowMapProperties.Height = 4096;
        shadowMapProperties.Wrapping = TextureWrap::ClampToEdge;
        shadowMapProperties.BorderColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

        s_RendererData.ShadowMapFramebuffer = Framebuffer::Create(4096, 4096);
        for (int i = 0; i < 4; i++)
        {
            s_RendererData.DirectionalShadowMapTextures[i] = Texture2D::Create(shadowMapProperties);
        }

        s_RendererData.SceneRenderDataUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::RenderData), 1);

        Ref<Shader> missingShader = CreateRef<Shader>("MissingShader", std::string(missingShaderSource));
        s_RendererData.DefaultMaterial = ShaderMaterial::Create("Missing Material", missingShader);

        // TODO: This is a hack to get the missing mesh add it to the PrimitiveMesh class
        Ref<Model> m = Model::Load("assets/models/MissingMesh.glb");
        s_RendererData.MissingMesh = m->GetMeshes()[0];

        s_ScreenQuad = PrimitiveMesh::CreateQuad();

        //s_FogShader = CreateRef<Shader>("FogShader", std::string(fogShaderSource));
        s_FogShader = CreateRef<Shader>("assets/shaders/FogShader.glsl");
        s_ToneMappingShader = CreateRef<Shader>("ToneMappingShader", std::string(toneMappingShaderSource));
        s_FXAAShader = CreateRef<Shader>("assets/shaders/FXAAShader.glsl"); // Shader source is too large
        s_FinalPassShader = CreateRef<Shader>("FinalPassShader", std::string(finalPassShaderSource));

        s_BloomShader = CreateRef<Shader>("assets/shaders/BloomShader.glsl");

        TextureProperties bloomTextureProperties;
        bloomTextureProperties.srgb = false;
        bloomTextureProperties.GenerateMipmaps = true;
        bloomTextureProperties.Format = ImageFormat::RGB16F; // Use a floating point format for bloom
        bloomTextureProperties.Width = 1280; // Initial size, will be resized later
        bloomTextureProperties.Height = 720; // Initial size, will be resized later
        bloomTextureProperties.Wrapping = TextureWrap::ClampToEdge;
        bloomTextureProperties.MinFilter = TextureFilter::LinearMipmapLinear;
        bloomTextureProperties.MagFilter = TextureFilter::Linear;

        s_BloomDownsampleTexture = Texture2D::Create(bloomTextureProperties);
        s_BloomUpsampleTexture = Texture2D::Create(bloomTextureProperties);
        
        s_BloomFramebuffer = Framebuffer::Create(1280, 720);
        GenerateBRDFLUT();
    }

    void Renderer3D::Shutdown()
    {
    }

    void Renderer3D::Submit(const LightComponent& light)
    {
        const int maxLights = Renderer3DData::MAX_LIGHTS - (light.type == LightComponent::Type::DirectionalLight ? 0 : 4);
        if (s_RendererData.RenderData.lightCount < maxLights)
        {
            s_RendererData.RenderData.lights[s_RendererData.RenderData.lightCount] = light;
            s_RendererData.RenderData.lightCount++;
        }
    }

    void Renderer3D::Submit(const RenderCommand& command)
    {
        if (!command.material)
        {
            s_RendererData.opaqueRenderQueue.push_back(command);
            return;
        }

        const auto& settings = command.material->GetRenderSettings();

        if (settings.transparencyMode == MaterialRenderSettings::TransparencyMode::Disabled)
        {
            s_RendererData.opaqueRenderQueue.push_back(command);
        }
        else
        {
            s_RendererData.transparentRenderQueue.push_back(command);
        }
    }

    // Temporal, this should be removed because this is rendering immediately.
    void Renderer3D::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform, uint32_t entityID)
    {
        shader->Bind();
        shader->setMat4("model", transform);
        shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(transform))));

        //REMOVE: This is for the first release of the engine it should be handled differently
        shader->setBool("showNormals", s_RenderSettings.showNormals);

        // Convert entityID to vec3
        uint32_t r = (entityID & 0x000000FF) >> 0;
        uint32_t g = (entityID & 0x0000FF00) >> 8;
        uint32_t b = (entityID & 0x00FF0000) >> 16;
        glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

        shader->setVec3("entityID", entityIDVec3);

        RendererAPI::DrawIndexed(vertexArray);

        s_Stats.DrawCalls++;
    }

    void Renderer3D::DepthPrePass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");
        forwardBuffer->Bind();


    }

    void Renderer3D::ShadowPass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        int directionalLightCount = 0;
    
        for (int i = 0; i < s_RendererData.RenderData.lightCount; ++i)
        {
            const auto& light = s_RendererData.RenderData.lights[i];
    
            // Check if the light is directional
            if (light.type == LightComponent::Type::DirectionalLight and light.Shadow)
            {
                auto& shadowMap = s_RendererData.DirectionalShadowMapTextures[directionalLightCount];
                s_RendererData.ShadowMapFramebuffer->AttachDepthTexture(shadowMap);
    
                s_RendererData.ShadowMapFramebuffer->Bind();
    
                RendererAPI::SetViewport(0, 0, 4096, 4096);
                RendererAPI::Clear();

                // Calculate light position based on camera and scene bounds
                glm::vec3 cameraPos = target->GetCameraTransform()[3];
                float shadowDistance = light.ShadowMaxDistance;
                
                // Position the light to cover the camera's view frustum
                glm::vec3 lightPos = cameraPos - light.Direction * (shadowDistance * 0.5f);

                // Adjust the orthographic projection bounds based on ShadowMaxDistance
                float orthoBounds = shadowDistance * 0.5f; // Slightly larger to avoid edge artifacts
                float nearPlane = 0.1f;
                float farPlane = shadowDistance;

                glm::mat4 lightProjection = glm::ortho(
                    -orthoBounds, orthoBounds, // left, right
                    -orthoBounds, orthoBounds, // bottom, top
                    nearPlane, farPlane        // near, far
                );

                glm::mat4 lightView = glm::lookAt(
                    lightPos,
                    lightPos + light.Direction,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                glm::mat4 lightSpaceMatrix = lightProjection * lightView;

                // Store the light space matrix for use in forward pass
                s_RendererData.RenderData.LightSpaceMatrices[directionalLightCount] = lightSpaceMatrix;

                depthShader->Bind();
                depthShader->setMat4("projView", lightSpaceMatrix);

                RendererAPI::SetCullFace(CullFace::Front);
    
                for (const auto& command : s_RendererData.opaqueRenderQueue)
                {
                    if (command.animator)
                        AnimationSystem::SetBoneTransformations(depthShader, command.animator);
                    else
                        depthShader->setBool("animated", false);

                    // Set the model matrix
                    depthShader->setMat4("model", command.transform);

                    Mesh* mesh = command.mesh.get();
                    
                    if(mesh == nullptr)
                    {
                        mesh = s_RendererData.MissingMesh.get();
                    }
                    
                    RendererAPI::DrawIndexed(mesh->GetVertexArray());
                }

                RendererAPI::SetCullFace(CullFace::Back);
    
                s_RendererData.ShadowMapFramebuffer->UnBind();
    
                directionalLightCount++;
    
                // Stop after processing the first 4 directional lights
                if (directionalLightCount >= Renderer3DData::MAX_DIRECTIONAL_SHADOWS)
                    break;
            }
        }

        // Update the uniform buffer with the light data
        s_RendererData.SceneRenderDataUniformBuffer->SetData(&s_RendererData.RenderData, sizeof(Renderer3DData::RenderData));
    }

    void Renderer3D::ForwardPass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");

        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        RendererAPI::SetClearColor({0.03f,0.03f,0.03f,1.0});
        RendererAPI::Clear();
        
        forwardBuffer->GetColorAttachment(1)->Clear({-1.0f,0.0f,0.0f,0.0f}); //TODO: This should only be done in the editor

        if (!s_RendererData.EnvironmentMap)
        {
            s_RendererData.EnvironmentMap = s_RendererData.DefaultSkybox;
        }

        // Bind the irradiance map
        s_RendererData.EnvironmentMap->BindIrradianceMap(6);
        s_RendererData.EnvironmentMap->BindPrefilteredMap(7);
        
        // Bind the BRDF LUT
        s_RendererData.BRDFLUT->Bind(8);

        // Set shadow map textures
        for (int i = 0; i < Renderer3DData::MAX_DIRECTIONAL_SHADOWS; ++i)
        {
            s_RendererData.DirectionalShadowMapTextures[i]->Bind(9 + i);
        }

        // Sort the render queue based on material and mesh
        std::sort(s_RendererData.opaqueRenderQueue.begin(), s_RendererData.opaqueRenderQueue.end(), [](const RenderCommand& a, const RenderCommand& b) {
            return std::tie(a.material, a.mesh) < std::tie(b.material, b.mesh);
        });

        for(const auto& command : s_RendererData.opaqueRenderQueue)
        {
            Material* material = command.material.get();

            if(material == nullptr or material->GetShader() == nullptr)
            {
                material = s_RendererData.DefaultMaterial.get();
            }
            
            material->Use();

            const Ref<Shader>& shader = material->GetShader();

            shader->Bind();

            // Set the irradiance map, is 6 because the first 6 slots are used by the material
            shader->setInt("irradianceMap", 6);
            shader->setInt("prefilterMap", 7);
            shader->setInt("brdfLUT", 8);

            // Set shadow map textures
            for (int i = 0; i < Renderer3DData::MAX_DIRECTIONAL_SHADOWS; ++i)
            {
                shader->setInt("shadowMaps[" + std::to_string(i) + "]", 9 + i);
            }

            if (command.animator)
                AnimationSystem::SetBoneTransformations(shader, command.animator);
            else
                shader->setBool("animated", false);

            shader->setMat4("model", command.transform);
            shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(command.transform))));

            //REMOVE: This is for the first release of the engine it should be handled differently
            shader->setBool("showNormals", s_RenderSettings.showNormals);

            // Convert entityID to vec3
            uint32_t r = (command.entityID & 0x000000FF) >> 0;
            uint32_t g = (command.entityID & 0x0000FF00) >> 8;
            uint32_t b = (command.entityID & 0x00FF0000) >> 16;
            glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

            shader->setVec3("entityID", entityIDVec3);

            Mesh* mesh = command.mesh.get();
            
            if(mesh == nullptr)
            {
                mesh = s_RendererData.MissingMesh.get();
            }

            // Apply material settings
            const MaterialRenderSettings& settings = material->GetRenderSettings();
            switch (settings.cullMode)
            {
                case MaterialRenderSettings::CullMode::Front:
                    RendererAPI::SetCullFace(CullFace::Front);
                    break;
                case MaterialRenderSettings::CullMode::Back:
                    RendererAPI::SetCullFace(CullFace::Back);
                    break;
                case MaterialRenderSettings::CullMode::None:
                    RendererAPI::SetFaceCulling(false);
                    break;
            }

            if (settings.depthTest)
            {
                RendererAPI::SetDepthMask(true);
            }
            else
            {
                RendererAPI::SetDepthMask(false);
            }

            if (settings.wireframe)
            {
                RendererAPI::SetPolygonMode(PolygonMode::Line);
            }
            else
            {
                RendererAPI::SetPolygonMode(PolygonMode::Fill);
            }

            RendererAPI::DrawIndexed(mesh->GetVertexArray());

            
            s_Stats.DrawCalls++;

            s_Stats.VertexCount += mesh->GetVertices().size();
            s_Stats.IndexCount += mesh->GetIndices().size();
        }

        forwardBuffer->UnBind();

        // Reset render settings to default
        RendererAPI::SetCullFace(CullFace::Back);
        RendererAPI::SetFaceCulling(true);
        RendererAPI::SetDepthMask(true);
        RendererAPI::SetPolygonMode(PolygonMode::Fill);
    }

    void Renderer3D::SkyboxPass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        // TODO: Think if this should be done here another time
        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");

        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1});

        RendererAPI::SetDepthMask(false);
        s_RendererData.EnvironmentMap->Bind(0);
        s_SkyboxShader->Bind();
        s_SkyboxShader->setInt("skybox", 0);
        s_SkyboxShader->setFloat("exposure", s_RenderSettings.EnvironmentExposure);
        RendererAPI::DrawIndexed(s_CubeMesh->GetVertexArray());
        RendererAPI::SetDepthMask(true);

        forwardBuffer->UnBind();
    }

    void Renderer3D::TransparentPass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");
        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        // Bind the irradiance map
        s_RendererData.EnvironmentMap->BindIrradianceMap(6);
        s_RendererData.EnvironmentMap->BindPrefilteredMap(7);

        // Bind the BRDF LUT
        s_RendererData.BRDFLUT->Bind(8);

        // Render transparent objects (back to front)
        glm::vec3 cameraPos = target->GetCameraTransform()[3];
        std::sort(s_RendererData.transparentRenderQueue.begin(), s_RendererData.transparentRenderQueue.end(), [&cameraPos](const RenderCommand& a, const RenderCommand& b) {
            float distA = glm::length(cameraPos - glm::vec3(a.transform[3]));
            float distB = glm::length(cameraPos - glm::vec3(b.transform[3]));
            return distA > distB;
        });

        RendererAPI::SetDepthMask(false);

        for (const auto& command : s_RendererData.transparentRenderQueue)
        {
            Material* material = command.material.get();

            if(material == nullptr)
            {
                material = s_RendererData.DefaultMaterial.get();
            }

            material->Use();

            const Ref<Shader>& shader = material->GetShader();

            shader->Bind();

            // Set the irradiance map, is 6 because the first 6 slots are used by the material
            shader->setInt("irradianceMap", 6);
            shader->setInt("prefilterMap", 7);
            shader->setInt("brdfLUT", 8);

            if (command.animator)
                AnimationSystem::SetBoneTransformations(shader, command.animator);
            else
                shader->setBool("animated", false);

            shader->setMat4("model", command.transform);
            shader->setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(command.transform))));

            //REMOVE: This is for the first release of the engine it should be handled differently
            shader->setBool("showNormals", s_RenderSettings.showNormals);

            // Convert entityID to vec3
            uint32_t r = (command.entityID & 0x000000FF) >> 0;
            uint32_t g = (command.entityID & 0x0000FF00) >> 8;
            uint32_t b = (command.entityID & 0x00FF0000) >> 16;
            glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

            shader->setVec3("entityID", entityIDVec3);

            Mesh* mesh = command.mesh.get();
            
            if(mesh == nullptr)
            {
                mesh = s_RendererData.MissingMesh.get();
            }

                // Apply material settings
                const MaterialRenderSettings& settings = material->GetRenderSettings();
                switch (settings.cullMode)
                {
                    case MaterialRenderSettings::CullMode::Front:
                        RendererAPI::SetCullFace(CullFace::Front);
                        break;
                    case MaterialRenderSettings::CullMode::Back:
                        RendererAPI::SetCullFace(CullFace::Back);
                        break;
                    case MaterialRenderSettings::CullMode::None:
                        RendererAPI::SetFaceCulling(false);
                        break;
                }
    
                if (settings.wireframe)
                {
                    RendererAPI::SetPolygonMode(PolygonMode::Line);
                }
                else
                {
                    RendererAPI::SetPolygonMode(PolygonMode::Fill);
                }

            RendererAPI::DrawIndexed(mesh->GetVertexArray());
        }

        RendererAPI::SetDepthMask(true);

        forwardBuffer->UnBind();

        // Reset render settings to default
        RendererAPI::SetCullFace(CullFace::Back);
        RendererAPI::SetFaceCulling(true);
        RendererAPI::SetDepthMask(true);
        RendererAPI::SetPolygonMode(PolygonMode::Fill);
    }

    void Renderer3D::PostProcessingPass(const Ref<RenderTarget>& target)
    {
        ZoneScoped;

        //Render All the fancy effects :D
        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");
        
        Ref<Framebuffer> lastBuffer = target->GetFramebuffer("PostProcessingA");
        Ref<Framebuffer> postBuffer = target->GetFramebuffer("PostProcessingB");

        // Copy the forward buffer to the last buffer (think if is necessary)
        lastBuffer->Bind();
        s_FinalPassShader->Bind();
        s_FinalPassShader->setInt("screenTexture", 0);
        forwardBuffer->GetColorAttachment(0)->Bind(0);

        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());
        s_FinalPassShader->Unbind();
        lastBuffer->UnBind();

        std::swap(lastBuffer, postBuffer);

        // Depth Fog (Is possible that some uniforms are not needed)
        if (s_RenderSettings.DepthFog)
        {
            lastBuffer->Bind();
            s_FogShader->Bind();
            s_FogShader->setBool("DepthFog", s_RenderSettings.DepthFog);
            s_FogShader->setVec3("FogColor", s_RenderSettings.FogColor);
            s_FogShader->setFloat("FogDensity", s_RenderSettings.FogDensity);
            s_FogShader->setFloat("FogHeight", s_RenderSettings.FogHeight);
            s_FogShader->setFloat("FogHeightDensity", s_RenderSettings.FogHeightDensity);
            s_FogShader->setMat4("invProjection", glm::inverse(target->GetCamera().GetProjection()));
            s_FogShader->setMat4("invView", target->GetCameraTransform());
            s_FogShader->setInt("colorTexture", 0);
            s_FogShader->setInt("depthTexture", 1);
            postBuffer->GetColorAttachment(0)->Bind(0);
            forwardBuffer->GetDepthTexture()->Bind(1);

            RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());
            s_FogShader->Unbind();
            lastBuffer->UnBind();

            std::swap(lastBuffer, postBuffer);
        }

        // Bloom
        if (s_RenderSettings.Bloom)
        {
            // Bind the framebuffer for downsampling with a color texture
            // Iterate over all the downsampling passes, binding each mip as output and the previous mip as input
            // In other framebuffer (output), we set as input the texture of the downsampling pass and

            glm::vec2 currentTargetSize = target->GetSize()/*  * 0.75f */;

            // TODO: Do it only when the resolution changes not every frame
            //s_BloomFramebuffer->Resize(currentTargetSize.x, currentTargetSize.y);
            static glm::vec2 lastTargetSize = {0, 0};
            if (currentTargetSize != lastTargetSize)
            {
                lastTargetSize = currentTargetSize;
                s_BloomDownsampleTexture->Resize(currentTargetSize.x, currentTargetSize.y);
                s_BloomUpsampleTexture->Resize(currentTargetSize.x, currentTargetSize.y);
            }

            s_BloomShader->Bind();
            s_BloomShader->setInt("sourceTexture", 0);
            postBuffer->GetColorAttachment(0)->Bind(0); // Bind the post-processing texture as the source
            s_BloomShader->setInt("downsamplingTexture", 1);
            s_BloomDownsampleTexture->Bind(1); // Bind the downsample texture to texture unit 1
            s_BloomShader->setInt("upsamplingTexture", 2);
            s_BloomUpsampleTexture->Bind(2); // Bind the upsample texture to texture unit 2

            // Copy the scene texture to the bloom downsample texture
            s_BloomShader->setInt("mode", 0); // 0 for copy
            s_BloomShader->setInt("mipmapLevel", 0); // Use mip level 0 for the initial copy
            s_BloomShader->setFloat("bloomIntensity", s_RenderSettings.BloomIntensity);

            s_BloomFramebuffer->AttachColorTexture(0, s_BloomDownsampleTexture, 0);
            s_BloomFramebuffer->Bind();
            s_BloomFramebuffer->SetDrawBuffers({0});
            RendererAPI::SetViewport(0, 0, currentTargetSize.x, currentTargetSize.y);
            RendererAPI::Clear();
            
            RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

            // Downsampling Passes
            int maxMipLevel = s_RenderSettings.BloomMaxMipLevels; // Number of downsampling passes
            for (int mip = 1; mip < maxMipLevel; mip++)
            {
                // Resize the bloom downsample texture for the current mip level
                uint32_t mipWidth = static_cast<uint32_t>(currentTargetSize.x) >> mip;
                uint32_t mipHeight = static_cast<uint32_t>(currentTargetSize.y) >> mip;

                // Attach the current mip level to the framebuffer
                s_BloomFramebuffer->AttachColorTexture(0, s_BloomDownsampleTexture, mip);
                s_BloomFramebuffer->Bind();

                RendererAPI::SetViewport(0, 0, mipWidth, mipHeight);
                RendererAPI::Clear();

                // Set the shader for downsampling
                s_BloomShader->setInt("mode", 1); // 1 for downsampling
                s_BloomShader->setInt("mipmapLevel", mip);

                RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

                s_BloomFramebuffer->UnBind();
            }

            // Copy the last downsampled texture to the upsample texture
            s_BloomFramebuffer->AttachColorTexture(0, s_BloomUpsampleTexture, maxMipLevel - 1);
            s_BloomFramebuffer->Bind();
            uint32_t mipWidth = static_cast<uint32_t>(currentTargetSize.x) >> (maxMipLevel - 1);
            uint32_t mipHeight = static_cast<uint32_t>(currentTargetSize.y) >> (maxMipLevel - 1);
            RendererAPI::SetViewport(0, 0, mipWidth, mipHeight);
            RendererAPI::Clear();
            s_BloomShader->setInt("mode", 0); // 0 for copy
            s_BloomShader->setInt("mipmapLevel", maxMipLevel - 1); // Use the last downsampled mip level
            s_BloomDownsampleTexture->Bind(0); // Bind the last downsampled texture to texture unit 0

            RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

            s_BloomShader->setFloat("filterRadius", s_RenderSettings.BloomRadius); // Set a filter radius for the bloom effect

            //RendererAPI::SetBlendFunc(BlendFunc::One, BlendFunc::One);

            // Upsampling Passes
            for (int mip = maxMipLevel - 2; mip >= 0; --mip)
            {
                // Resize the bloom upsample texture for the current mip level
                uint32_t mipWidth = static_cast<uint32_t>(currentTargetSize.x) >> mip;
                uint32_t mipHeight = static_cast<uint32_t>(currentTargetSize.y) >> mip;

                // Attach the current mip level to the framebuffer
                s_BloomFramebuffer->AttachColorTexture(0, s_BloomUpsampleTexture, mip);
                s_BloomFramebuffer->Bind();

                RendererAPI::SetViewport(0, 0, mipWidth, mipHeight);
                RendererAPI::Clear();

                // Set the shader for upsampling
                s_BloomShader->setInt("mode", 2); // 2 for upsampling
                s_BloomShader->setInt("mipmapLevel", mip);

                RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

                s_BloomFramebuffer->UnBind();
            }

            //RendererAPI::SetBlendFunc(BlendFunc::SrcAlpha, BlendFunc::OneMinusSrcAlpha);

            // Final Composition Pass
            lastBuffer->Bind();
            s_BloomShader->setInt("mode", 3); // 3 for final composition

            RendererAPI::SetViewport(0, 0, static_cast<uint32_t>(target->GetSize().x), static_cast<uint32_t>(target->GetSize().y));
            RendererAPI::Clear();

            RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

            s_BloomShader->Unbind();
            s_BloomFramebuffer->UnBind();
            lastBuffer->UnBind();
            std::swap(lastBuffer, postBuffer);
        }

        //ToneMapping
        lastBuffer->Bind();
        s_ToneMappingShader->Bind();
        s_ToneMappingShader->setInt("screenTexture", 0);
        s_ToneMappingShader->setFloat("exposure", s_RenderSettings.Exposure);
        postBuffer->GetColorAttachment(0)->Bind(0);

        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

        s_ToneMappingShader->Unbind();
        lastBuffer->UnBind();

        std::swap(lastBuffer, postBuffer);

        // TODO better logic for dynamically enabling and disabling individual post-processing effects
        // Fast aproXimate AntiAliasing
        if (s_RenderSettings.FXAA)
        {

            lastBuffer->Bind();
            s_FXAAShader->Bind();
            s_FXAAShader->setInt("screenTexture", 0);
            s_FXAAShader->setVec2("screenSize", {forwardBuffer->GetWidth(), forwardBuffer->GetHeight()});
            postBuffer->GetColorAttachment(0)->Bind(0);

            RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

            s_FXAAShader->Unbind();
            lastBuffer->UnBind();

            std::swap(lastBuffer, postBuffer);
        }

        // Final pass to copy the post-processing texture to the main render texture

        //This has to be set because the s_ScreenQuad overwrites the depth buffer
        RendererAPI::SetDepthMask(false);

        // Copy PostProcessing Texture to the Main Render Texture
        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0});

        s_FinalPassShader->Bind();
        s_FinalPassShader->setInt("screenTexture", 0);
        postBuffer->GetColorAttachment(0)->Bind(0);
        //s_BloomDownsampleTexture->Bind(0);
        //s_BloomUpsampleTexture->Bind(0); // Use the downsampled texture for final pass

        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

        s_FinalPassShader->Unbind();

        RendererAPI::SetDepthMask(true);

        forwardBuffer->UnBind();
    }

    void Renderer3D::ResetCalls()
    {
        s_RendererData.RenderData.lightCount = 0;
        s_RendererData.opaqueRenderQueue.clear();
        s_RendererData.transparentRenderQueue.clear();

        s_RendererData.EnvironmentMap = nullptr;
    }

    void Renderer3D::GenerateBRDFLUT()
    {
        TextureProperties properties;
        properties.Format = ImageFormat::RGBA16F;
        properties.Width = 512;
        properties.Height = 512;
        properties.GenerateMipmaps = false;
        properties.Wrapping = TextureWrap::ClampToEdge;
        properties.MinFilter = TextureFilter::Linear;
        properties.MagFilter = TextureFilter::Linear;

        s_RendererData.BRDFLUT = Texture2D::Create(properties);
        
        Framebuffer framebuffer = Framebuffer(properties.Width, properties.Height);
        framebuffer.AttachColorTexture(0, s_RendererData.BRDFLUT);
        framebuffer.Bind();
        framebuffer.SetDrawBuffers({0});

        RendererAPI::SetViewport(0, 0, properties.Width, properties.Height);

        brdfShader->Bind();

        RendererAPI::SetClearColor({0.0f, 0.0f, 0.0f, 1.0f});
        RendererAPI::Clear();

        s_ScreenQuad->GetVertexArray()->Bind();
        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

        framebuffer.UnBind();
    }
}