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
#include "CoffeeEngine/Embedded/ShadowShader.inl"

#include <cstdint>
#include <glm/fwd.hpp>
#include <glm/matrix.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    Renderer3DData Renderer3D::s_RendererData;
    Renderer3DStats Renderer3D::s_Stats;
    Renderer3DSettings Renderer3D::s_RenderSettings;

    Ref<Mesh> Renderer3D::s_ScreenQuad;

    Ref<Shader> Renderer3D::s_ToneMappingShader;
    Ref<Shader> Renderer3D::s_FinalPassShader;

    static Ref<Cubemap> s_EnvironmentMap;
    static Ref<Mesh> s_SkyboxMesh;
    static Ref<Shader> s_SkyboxShader;

    void Renderer3D::Init()
    {
        ZoneScoped;

        s_EnvironmentMap = Cubemap::Load("assets/textures/StandardCubeMap.hdr");

        s_SkyboxMesh = PrimitiveMesh::CreateCube({-1.0f, -1.0f, -1.0f});

        s_SkyboxShader = CreateRef<Shader>("assets/shaders/SkyboxShader.glsl");
        s_SkyboxShader->Bind();
        s_SkyboxShader->setInt("skybox", 0);

        // Shadow map
        s_RendererData.ShadowMapFramebuffer = Framebuffer::Create(4096, 4096, {});
        for (int i = 0; i < 4; i++)
        {
            s_RendererData.DirectionalShadowMapTextures[i] = Texture2D::Create(4096, 4096, ImageFormat::DEPTH24STENCIL8);
        }

        s_RendererData.SceneRenderDataUniformBuffer = UniformBuffer::Create(sizeof(Renderer3DData::RenderData), 1);

        Ref<Shader> missingShader = CreateRef<Shader>("MissingShader", std::string(missingShaderSource));
        s_RendererData.DefaultMaterial = ShaderMaterial::Create("Missing Material", missingShader);
        //s_RendererData.DefaultMaterial = nullptr;/* CreateRef<Material>("Missing Material", missingShader); //TODO: Port it to use the Material::Create and use ShaderMaterial */

        // TODO: This is a hack to get the missing mesh add it to the PrimitiveMesh class
        Ref<Model> m = Model::Load("assets/models/MissingMesh.glb");
        s_RendererData.MissingMesh = m->GetMeshes()[0];

        s_ScreenQuad = PrimitiveMesh::CreateQuad();

        s_ToneMappingShader = CreateRef<Shader>("ToneMappingShader", std::string(toneMappingShaderSource));
        s_FinalPassShader = CreateRef<Shader>("FinalPassShader", std::string(finalPassShaderSource));
    }

    void Renderer3D::Shutdown()
    {
    }

    void Renderer3D::Submit(const LightComponent& light)
    {
        s_RendererData.RenderData.lights[s_RendererData.RenderData.lightCount] = light;
        s_RendererData.RenderData.lightCount++;
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

    void Renderer3D::ShadowPass(const RenderTarget& target)
    {
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

                // Adjust the orthographic projection bounds based on ShadowMaxDistance
                float orthoBounds = light.ShadowMaxDistance * 0.5f;
                float nearPlane = 0.1f;
                float farPlane = light.ShadowMaxDistance;

                glm::mat4 lightProjection = glm::ortho(
                    -orthoBounds, orthoBounds, // left, right
                    -orthoBounds, orthoBounds, // bottom, top
                    nearPlane, farPlane        // near, far
                );

                glm::mat4 lightView = glm::lookAt(
                    light.Position,
                    light.Position + light.Direction,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                glm::mat4 lightSpaceMatrix = lightProjection * lightView;

                static Ref<Shader> shadowShader = CreateRef<Shader>("ShadowShader", std::string(shadowShaderSource));
                shadowShader->Bind();
                shadowShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

                RendererAPI::SetCullFace(CullFace::Front);
    
                for (const auto& command : s_RendererData.opaqueRenderQueue)
                {
                    // Set the model matrix
                    shadowShader->setMat4("model", command.transform);

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
    }

    void Renderer3D::ForwardPass(const RenderTarget& target)
    {
        ZoneScoped;

        // TODO: Think if this should be done here
        s_RendererData.SceneRenderDataUniformBuffer->SetData(&s_RendererData.RenderData, sizeof(Renderer3DData::RenderData));

        const Ref<Framebuffer>& forwardBuffer = target.GetFramebuffer("Forward");

        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        RendererAPI::SetClearColor({0.03f,0.03f,0.03f,1.0});
        RendererAPI::Clear();
        
        forwardBuffer->GetColorTexture("EntityID")->Clear({-1.0f,0.0f,0.0f,0.0f}); //TODO: This should only be done in the editor

        // Bind the irradiance map
        s_EnvironmentMap->BindIrradianceMap(6);
        s_EnvironmentMap->BindPrefilteredMap(7);
        s_EnvironmentMap->BindBRDFLUT(8);

        // TEMPORAL: lightSpaceMatrix array for shadow mapping
        glm::mat4 lightSpaceMatrices[Renderer3DData::MAX_DIRECTIONAL_SHADOWS];
        int directionalLightCount = 0;
        for (int i = 0; i < s_RendererData.RenderData.lightCount; ++i)
        {
            const auto& light = s_RendererData.RenderData.lights[i];

            // Check if the light is directional
            if (light.type == LightComponent::Type::DirectionalLight)
            {
                // Adjust the orthographic projection bounds based on ShadowMaxDistance
                float orthoBounds = light.ShadowMaxDistance * 0.5f;
                float nearPlane = 0.1f;
                float farPlane = light.ShadowMaxDistance;

                glm::mat4 lightProjection = glm::ortho(
                    -orthoBounds, orthoBounds, // left, right
                    -orthoBounds, orthoBounds, // bottom, top
                    nearPlane, farPlane        // near, far
                );

                glm::mat4 lightView = glm::lookAt(
                    light.Position,
                    light.Position + light.Direction,
                    glm::vec3(0.0f, 1.0f, 0.0f)
                );

                lightSpaceMatrices[directionalLightCount] = lightProjection * lightView;
                
                directionalLightCount++;
            }
            // Stop after processing the first 4 directional lights
            if (directionalLightCount >= Renderer3DData::MAX_DIRECTIONAL_SHADOWS)
                break;
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

            // Set the light space matrices for shadow mapping
            for (int i = 0; i < Renderer3DData::MAX_DIRECTIONAL_SHADOWS; ++i)
            {
                shader->setMat4("lightSpaceMatrices[" + std::to_string(i) + "]", lightSpaceMatrices[i]);
            }

            // Set shadow map textures
            for (int i = 0; i < Renderer3DData::MAX_DIRECTIONAL_SHADOWS; ++i)
            {
                s_RendererData.DirectionalShadowMapTextures[i]->Bind(9 + i);
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

    void Renderer3D::SkyboxPass(const RenderTarget& target)
    {
        ZoneScoped;

        // TODO: Think if this should be done here another time
        const Ref<Framebuffer>& forwardBuffer = target.GetFramebuffer("Forward");

        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1});

        RendererAPI::SetDepthMask(false);
        s_EnvironmentMap->Bind(0);
        s_SkyboxShader->Bind();
        RendererAPI::DrawIndexed(s_SkyboxMesh->GetVertexArray());
        RendererAPI::SetDepthMask(true);

        forwardBuffer->UnBind();
    }

    void Renderer3D::TransparentPass(const RenderTarget& target)
    {
        ZoneScoped;

        const Ref<Framebuffer>& forwardBuffer = target.GetFramebuffer("Forward");
        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        // Bind the irradiance map
        s_EnvironmentMap->BindIrradianceMap(6);
        s_EnvironmentMap->BindPrefilteredMap(7);
        s_EnvironmentMap->BindBRDFLUT(8);

        // Render transparent objects (back to front)
        glm::vec3 cameraPos = target.GetCameraTransform()[3];
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

    void Renderer3D::PostProcessingPass(const RenderTarget &target)
    {
        ZoneScoped;

        //Render All the fancy effects :D
        const Ref<Framebuffer>& forwardBuffer = target.GetFramebuffer("Forward");
        const Ref<Framebuffer>& postBuffer = target.GetFramebuffer("PostProcessing");
        postBuffer->Bind();

        //ToneMapping

        s_ToneMappingShader->Bind();
        s_ToneMappingShader->setInt("screenTexture", 0);
        s_ToneMappingShader->setFloat("exposure", s_RenderSettings.Exposure);
        forwardBuffer->GetColorTexture("Color")->Bind(0);

        RendererAPI::DrawIndexed(s_ScreenQuad->GetVertexArray());

        s_ToneMappingShader->Unbind();

        //This has to be set because the s_ScreenQuad overwrites the depth buffer
        RendererAPI::SetDepthMask(false);

        // Copy PostProcessing Texture to the Main Render Texture
        forwardBuffer->Bind();
        forwardBuffer->SetDrawBuffers({0});

        s_FinalPassShader->Bind();
        s_FinalPassShader->setInt("screenTexture", 0);
        postBuffer->GetColorTexture("Color")->Bind(0);

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
    }
}