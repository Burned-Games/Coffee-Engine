#include "Renderer2D.h"
#include "CoffeeEngine/Math/BoundingBox.h"
#include "CoffeeEngine/Math/Frustum.h"
#include "CoffeeEngine/Renderer/MSDFData.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Renderer/VertexArray.h"
#include "CoffeeEngine/Renderer/RendererAPI.h"

#include "CoffeeEngine/Embedded/QuadShader.inl"
#include "CoffeeEngine/Embedded/TextShader.inl"
#include "CoffeeEngine/Embedded/LineShader.inl"

#include <array>
#include <cstddef>
#include <glm/fwd.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include <cstdint>
#include <queue>
#include <vector>

namespace Coffee {

    struct QuadVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        
        float TexIndex;
        float TilingFactor;

        glm::vec3 EntityID;
    };

    struct LineVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        
        glm::vec3 EntityID;
    };

    struct TextVertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;

        glm::vec3 EntityID;
    };

    struct Batch
    {
        static const uint32_t MaxQuadCount = 20000; // Think of increasing this number to 20000
        static const uint32_t MaxVertices = MaxQuadCount * 4;
        static const uint32_t MaxIndices = MaxQuadCount * 6;
        static const uint32_t MaxTextureSlots = 64;

        std::vector<QuadVertex> QuadVertices;
        uint32_t QuadIndexCount = 0;

        std::vector<LineVertex> LineVertices;

        std::vector<TextVertex> TextVertices;
        uint32_t TextIndexCount = 0;

        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
        uint32_t TextureSlotIndex = 1; // 0 is reserved for white texture

        Ref<Texture2D> FontAtlasTexture;

        float LineWidth = 1.5f;
    };

    struct Renderer2DData
    {
        std::queue<Batch> WorldBatches;
        std::queue<Batch> ScreenBatches;

        Ref<VertexArray> QuadVertexArray;
        Ref<VertexBuffer> QuadVertexBuffer;

        Ref<VertexArray> LineVertexArray;
        Ref<VertexBuffer> LineVertexBuffer;

        Ref<VertexArray> TextVertexArray;
        Ref<VertexBuffer> TextVertexBuffer;

        Ref<Shader> QuadShader;
        Ref<Shader> LineShader;
        Ref<Shader> TextShader;

        Ref<Texture2D> WhiteTexture;

        glm::vec4 QuadVertexPositions[4] = {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.5f, -0.5f, 0.0f, 1.0f},
            {0.5f, 0.5f, 0.0f, 1.0f},
            {-0.5f, 0.5f, 0.0f, 1.0f}
        };

    };

    static Renderer2DData s_Renderer2DData;

    void Renderer2D::Init()
    {
        s_Renderer2DData.QuadVertexArray = VertexArray::Create();

        s_Renderer2DData.QuadVertexBuffer = VertexBuffer::Create(Batch::MaxVertices * sizeof(QuadVertex));
        BufferLayout quadLayout = {
            {ShaderDataType::Vec3, "a_Position"},
            {ShaderDataType::Vec4, "a_Color"},
            {ShaderDataType::Vec2, "a_TexCoord"},
            {ShaderDataType::Float, "a_TexIndex"},
            {ShaderDataType::Float, "a_TilingFactor"},
            {ShaderDataType::Vec3, "a_EntityID"}
        };
        s_Renderer2DData.QuadVertexBuffer->SetLayout(quadLayout);
        s_Renderer2DData.QuadVertexArray->AddVertexBuffer(s_Renderer2DData.QuadVertexBuffer);

        std::vector<uint32_t> quadIndices(Batch::MaxIndices);

        uint32_t offset = 0;
        for(uint32_t i = 0; i < Batch::MaxIndices; i += 6)
        {
            quadIndices[i + 0] = offset + 0;
            quadIndices[i + 1] = offset + 1;
            quadIndices[i + 2] = offset + 2;

            quadIndices[i + 3] = offset + 2;
            quadIndices[i + 4] = offset + 3;
            quadIndices[i + 5] = offset + 0;

            offset += 4;
        }

        Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices.data(), quadIndices.size());
        s_Renderer2DData.QuadVertexArray->SetIndexBuffer(quadIB);
        quadIndices.clear();

        // Line
        s_Renderer2DData.LineVertexArray = VertexArray::Create();

        s_Renderer2DData.LineVertexBuffer = VertexBuffer::Create(Batch::MaxVertices * sizeof(LineVertex));
        BufferLayout lineLayout = {
            {ShaderDataType::Vec3, "a_Position"},
            {ShaderDataType::Vec4, "a_Color"},
            {ShaderDataType::Vec3, "a_EntityID"}
        };
        s_Renderer2DData.LineVertexBuffer->SetLayout(lineLayout);
        s_Renderer2DData.LineVertexArray->AddVertexBuffer(s_Renderer2DData.LineVertexBuffer);

        // Text
        s_Renderer2DData.TextVertexArray = VertexArray::Create();

        s_Renderer2DData.TextVertexBuffer = VertexBuffer::Create(Batch::MaxVertices * sizeof(TextVertex));
        BufferLayout textLayout = {
            {ShaderDataType::Vec3, "a_Position"},
            {ShaderDataType::Vec4, "a_Color"},
            {ShaderDataType::Vec2, "a_TexCoord"},
            {ShaderDataType::Vec3, "a_EntityID"}
        };
        s_Renderer2DData.TextVertexBuffer->SetLayout(textLayout);
        s_Renderer2DData.TextVertexArray->AddVertexBuffer(s_Renderer2DData.TextVertexBuffer);
        s_Renderer2DData.TextVertexArray->SetIndexBuffer(quadIB);

        s_Renderer2DData.WhiteTexture = Texture2D::Create(1, 1, ImageFormat::RGBA8);
        uint32_t whiteTextureData = 0xffffffff;
        s_Renderer2DData.WhiteTexture->SetData(&whiteTextureData, sizeof(uint32_t));

        s_Renderer2DData.QuadShader = CreateRef<Shader>("QuadShader", std::string(quadShaderSource));
        s_Renderer2DData.LineShader = CreateRef<Shader>("LineShader", std::string(lineShaderSource));
        s_Renderer2DData.TextShader = CreateRef<Shader>("TextShader", std::string(textShaderSource));
    }

    void Renderer2D::WorldPass(const Ref<RenderTarget>& target)
    {
        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");

        forwardBuffer->Bind();
        //forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        while (!s_Renderer2DData.WorldBatches.empty())
        {
            Batch& batch = s_Renderer2DData.WorldBatches.front();

            if(batch.QuadIndexCount > 0)
            {
                s_Renderer2DData.QuadVertexBuffer->SetData(batch.QuadVertices.data(), batch.QuadVertices.size() * sizeof(QuadVertex));

                batch.TextureSlots[0] = s_Renderer2DData.WhiteTexture;

                for(uint32_t i = 0; i < batch.TextureSlotIndex; i++)
                {
                    batch.TextureSlots[i]->Bind(i);
                }

                s_Renderer2DData.QuadShader->Bind();
                RendererAPI::DrawIndexed(s_Renderer2DData.QuadVertexArray, batch.QuadIndexCount);
            }

            if(batch.LineVertices.size() > 0)
            {
                s_Renderer2DData.LineVertexBuffer->SetData(batch.LineVertices.data(), batch.LineVertices.size() * sizeof(LineVertex));

                s_Renderer2DData.LineShader->Bind();
                RendererAPI::DrawLines(s_Renderer2DData.LineVertexArray, batch.LineVertices.size(), batch.LineWidth);
            }

            if(batch.TextIndexCount > 0)
            {
                s_Renderer2DData.TextVertexBuffer->SetData(batch.TextVertices.data(), batch.TextVertices.size() * sizeof(TextVertex));

                batch.FontAtlasTexture->Bind(0);

                s_Renderer2DData.TextShader->Bind();
                RendererAPI::DrawIndexed(s_Renderer2DData.TextVertexArray, batch.TextIndexCount);
            }

            s_Renderer2DData.WorldBatches.pop();
        }

        forwardBuffer->UnBind();
    }

    void Renderer2D::ScreenPass(const Ref<RenderTarget>& target)
    {
        // TODO: Modify the target to have a framebuffer for 2D elements
        const Ref<Framebuffer>& forwardBuffer = target->GetFramebuffer("Forward");

        forwardBuffer->Bind();
        //forwardBuffer->SetDrawBuffers({0, 1}); //TODO: This should only be done in the editor

        while (!s_Renderer2DData.ScreenBatches.empty())
        {
            Batch& batch = s_Renderer2DData.ScreenBatches.front();

            if(batch.QuadIndexCount > 0)
            {
                s_Renderer2DData.QuadVertexBuffer->SetData(batch.QuadVertices.data(), batch.QuadVertices.size() * sizeof(QuadVertex));

                batch.TextureSlots[0] = s_Renderer2DData.WhiteTexture;

                for(uint32_t i = 0; i < batch.TextureSlotIndex; i++)
                {
                    batch.TextureSlots[i]->Bind(i);
                }

                s_Renderer2DData.QuadShader->Bind();
                RendererAPI::DrawIndexed(s_Renderer2DData.QuadVertexArray, batch.QuadIndexCount);
            }

            if(batch.LineVertices.size() > 0)
            {
                s_Renderer2DData.LineVertexBuffer->SetData(batch.LineVertices.data(), batch.LineVertices.size() * sizeof(LineVertex));

                s_Renderer2DData.LineShader->Bind();
                RendererAPI::DrawLines(s_Renderer2DData.LineVertexArray, batch.LineVertices.size(), batch.LineWidth);
            }

            if(batch.TextIndexCount > 0)
            {
                s_Renderer2DData.TextVertexBuffer->SetData(batch.TextVertices.data(), batch.TextVertices.size() * sizeof(TextVertex));

                batch.FontAtlasTexture->Bind(0);

                s_Renderer2DData.TextShader->Bind();
                RendererAPI::DrawIndexed(s_Renderer2DData.TextVertexArray, batch.TextIndexCount);
            }

            s_Renderer2DData.ScreenBatches.pop();
        }

        forwardBuffer->UnBind();
    }

    void Renderer2D::Shutdown()
    {
    }

    void Renderer2D::DrawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, uint32_t entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f})
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawRect(transform, color, RenderMode::Screen, entityID);
    }

    void Renderer2D::DrawRect(const glm::vec2& position, const glm::vec2& size, const float rotation, const glm::vec4& color, uint32_t entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f})
                            * glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f})
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawRect(transform, color, RenderMode::Screen, entityID);
    }

    void Renderer2D::DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color, uint32_t entityID)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawRect(transform, color, RenderMode::World, entityID);
    }

    void Renderer2D::DrawRect(const glm::mat4& transform, const glm::vec4& color, RenderMode mode, uint32_t entityID)
    {
        constexpr size_t quadVertexCount = 4;
        constexpr glm::vec2 texCoords[] = {
            {0.0f, 0.0f},
            {1.0f, 0.0f},
            {1.0f, 1.0f},
            {0.0f, 1.0f}
        };

        Batch& batch = GetBatch(mode);

        if(batch.QuadIndexCount >= Batch::MaxIndices)
        {
            NextBatch(mode);
            batch = GetBatch(mode);
        }

        // Convert entityID to vec3
        uint32_t r = (entityID & 0x000000FF) >> 0;
        uint32_t g = (entityID & 0x0000FF00) >> 8;
        uint32_t b = (entityID & 0x00FF0000) >> 16;
        glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

        for(size_t i = 0; i < quadVertexCount; i++)
        {
            batch.QuadVertices.push_back(
            {
                transform * s_Renderer2DData.QuadVertexPositions[i], 
                color, 
                texCoords[i], 
                0.0f, 
                1.0f, 
                entityIDVec3
                });
        }

        batch.QuadIndexCount += 6;
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f})
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawQuad(transform, texture, tilingFactor, tintColor, RenderMode::Screen);
    }

    void Renderer2D::DrawQuad(const glm::vec2& position, const glm::vec2& size, const float rotation, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), {position.x, position.y, 0.0f})
                            * glm::rotate(glm::mat4(1.0f), rotation, {0.0f, 0.0f, 1.0f})
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawQuad(transform, texture, tilingFactor, tintColor, RenderMode::Screen);
    }

    void Renderer2D::DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor)
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), position)
                            * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});
        
        DrawQuad(transform, texture, tilingFactor, tintColor, RenderMode::World);
    }

    void Renderer2D::DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, RenderMode mode, uint32_t entityID, const glm::vec4& uvRect)
    {
        constexpr size_t quadVertexCount = 4;
        const glm::vec2 texCoords[] = {
            {uvRect.x, uvRect.y},
            {uvRect.x + uvRect.z, uvRect.y},
            {uvRect.x + uvRect.z, uvRect.y + uvRect.w},
            {uvRect.x, uvRect.y + uvRect.w}
        };

        Batch& batch = GetBatch(mode);

        if(batch.QuadIndexCount >= Batch::MaxIndices)
        {
            NextBatch(mode);
            batch = GetBatch(mode);
        }

        float textureIndex = 0.0f;
        for(uint32_t i = 1; i < batch.TextureSlotIndex; i++)
        {
            if(batch.TextureSlots[i] == texture)
            {
                textureIndex = (float)i;
                break;
            }
        }

        if(textureIndex == 0.0f)
        {
            if(batch.TextureSlotIndex >= Batch::MaxTextureSlots)
            {
                NextBatch(mode);
                batch = GetBatch(mode);
            }

            textureIndex = (float)batch.TextureSlotIndex;
            batch.TextureSlots[batch.TextureSlotIndex] = texture;
            batch.TextureSlotIndex++;
        }

        // Convert entityID to vec3
        uint32_t r = (entityID & 0x000000FF) >> 0;
        uint32_t g = (entityID & 0x0000FF00) >> 8;
        uint32_t b = (entityID & 0x00FF0000) >> 16;
        glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

        glm::vec4 quadVerts[4] = {
            s_Renderer2DData.QuadVertexPositions[0],
            s_Renderer2DData.QuadVertexPositions[1],
            s_Renderer2DData.QuadVertexPositions[2],
            s_Renderer2DData.QuadVertexPositions[3]
        };

        quadVerts[1].x = quadVerts[2].x = -0.5f + uvRect.z;
        quadVerts[2].y = quadVerts[3].y = -0.5f + uvRect.w;

        for(size_t i = 0; i < quadVertexCount; i++)
        {
            batch.QuadVertices.push_back(
            {
                transform * quadVerts[i],
                tintColor, 
                texCoords[i], 
                textureIndex, 
                tilingFactor, 
                entityIDVec3
                });
        }

        batch.QuadIndexCount += 6;
    }

    void Renderer2D::DrawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color, float linewidth)
    {
        Batch& batch = GetBatch(RenderMode::Screen);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::Screen);
            batch = GetBatch(RenderMode::Screen);
        }

/*         // Convert entityID to vec3
        uint32_t r = (4294967295 & 0x000000FF) >> 0;
        uint32_t g = (4294967295 & 0x0000FF00) >> 8;
        uint32_t b = (4294967295 & 0x00FF0000) >> 16;
        glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f); */

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        batch.LineVertices.push_back({glm::vec3(start, 0.0f), color, entityIDVec3});
        batch.LineVertices.push_back({glm::vec3(end, 0.0f), color, entityIDVec3});
    }

    void Renderer2D::DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color , float linewidth)
    {
        Batch& batch = GetBatch(RenderMode::World);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::World);
            batch = GetBatch(RenderMode::World);
        }

/*         // entt::null is 4294967295
        uint32_t r = (4294967295 & 0x000000FF) >> 0;
        uint32_t g = (4294967295 & 0x0000FF00) >> 8;
        uint32_t b = (4294967295 & 0x00FF0000) >> 16;
        glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f); */

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        batch.LineVertices.push_back({start, color, entityIDVec3});
        batch.LineVertices.push_back({end, color, entityIDVec3});
    }

    void Renderer2D::DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color , float linewidth)
    {
        Batch& batch = GetBatch(RenderMode::Screen);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::Screen);
            batch = GetBatch(RenderMode::Screen);
        }

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        const uint32_t segments = 32;
        const float increment = glm::two_pi<float>() / segments;

        for(uint32_t i = 0; i < segments; i++)
        {
            float angle0 = increment * i;
            float angle1 = increment * (i + 1);

            glm::vec2 start = position + glm::vec2(glm::cos(angle0), glm::sin(angle0)) * radius;
            glm::vec2 end = position + glm::vec2(glm::cos(angle1), glm::sin(angle1)) * radius;

            batch.LineVertices.push_back({glm::vec3(start, 0.0f), color, entityIDVec3});
            batch.LineVertices.push_back({glm::vec3(end, 0.0f), color, entityIDVec3});

            
        }
    }
    void Renderer2D::DrawCircle(const glm::vec3& position, float radius, const glm::quat& rotation, const glm::vec4& color , float linewidth)
    {
        Batch& batch = GetBatch(RenderMode::World);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::World);
            batch = GetBatch(RenderMode::World);
        }

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        const uint32_t segments = 32;
        const float increment = glm::two_pi<float>() / segments;

        for(uint32_t i = 0; i < segments; i++)
        {
            float angle0 = increment * i;
            float angle1 = increment * (i + 1);

            glm::vec3 start = position + glm::toMat3(rotation) * glm::vec3(glm::cos(angle0), glm::sin(angle0), 0.0f) * radius;
            glm::vec3 end = position + glm::toMat3(rotation) * glm::vec3(glm::cos(angle1), glm::sin(angle1), 0.0f) * radius;

            batch.LineVertices.push_back({start, color, entityIDVec3});
            batch.LineVertices.push_back({end, color, entityIDVec3});

            
        }
    }

    void Renderer2D::DrawSphere(const glm::vec3& position, float radius, const glm::quat& rotation, const glm::vec4& color , float linewidth)
    {
        DrawCircle(position, radius, rotation, color, linewidth);
        DrawCircle(position, radius, rotation * glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)), color, linewidth);
        DrawCircle(position, radius, rotation * glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f)), color, linewidth);
    }

    void Renderer2D::DrawBox(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& size, const glm::vec4& color , const bool& isCentered, float linewidth)
    {
        glm::vec3 halfSize = size * 0.5f;
        glm::vec3 vertices[8];

        if (isCentered) {
            vertices[0] = position + rotation * glm::vec3(-halfSize.x, -halfSize.y, -halfSize.z);
            vertices[1] = position + rotation * glm::vec3(halfSize.x, -halfSize.y, -halfSize.z);
            vertices[2] = position + rotation * glm::vec3(halfSize.x, halfSize.y, -halfSize.z);
            vertices[3] = position + rotation * glm::vec3(-halfSize.x, halfSize.y, -halfSize.z);
            vertices[4] = position + rotation * glm::vec3(-halfSize.x, -halfSize.y, halfSize.z);
            vertices[5] = position + rotation * glm::vec3(halfSize.x, -halfSize.y, halfSize.z);
            vertices[6] = position + rotation * glm::vec3(halfSize.x, halfSize.y, halfSize.z);
            vertices[7] = position + rotation * glm::vec3(-halfSize.x, halfSize.y, halfSize.z);
        } else {
            vertices[0] = position + rotation * glm::vec3(0.0f, 0.0f, 0.0f);
            vertices[1] = position + rotation * glm::vec3(size.x, 0.0f, 0.0f);
            vertices[2] = position + rotation * glm::vec3(size.x, size.y, 0.0f);
            vertices[3] = position + rotation * glm::vec3(0.0f, size.y, 0.0f);
            vertices[4] = position + rotation * glm::vec3(0.0f, 0.0f, size.z);
            vertices[5] = position + rotation * glm::vec3(size.x, 0.0f, size.z);
            vertices[6] = position + rotation * glm::vec3(size.x, size.y, size.z);
            vertices[7] = position + rotation * glm::vec3(0.0f, size.y, size.z);
        }

        DrawLine(vertices[0], vertices[1], color, linewidth);
        DrawLine(vertices[1], vertices[2], color, linewidth);
        DrawLine(vertices[2], vertices[3], color, linewidth);
        DrawLine(vertices[3], vertices[0], color, linewidth);

        DrawLine(vertices[4], vertices[5], color, linewidth);
        DrawLine(vertices[5], vertices[6], color, linewidth);
        DrawLine(vertices[6], vertices[7], color, linewidth);
        DrawLine(vertices[7], vertices[4], color, linewidth);

        DrawLine(vertices[0], vertices[4], color, linewidth);
        DrawLine(vertices[1], vertices[5], color, linewidth);
        DrawLine(vertices[2], vertices[6], color, linewidth);
        DrawLine(vertices[3], vertices[7], color, linewidth);
    }

    void Renderer2D::DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color , float linewidth)
    {
        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(max.x, min.y, min.z), color, linewidth);
        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color, linewidth);
        DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, min.z), color, linewidth);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, min.y, min.z), color, linewidth);

        DrawLine(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color, linewidth);
        DrawLine(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color, linewidth);
        DrawLine(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color, linewidth);
        DrawLine(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, min.y, max.z), color, linewidth);

        DrawLine(glm::vec3(min.x, min.y, min.z), glm::vec3(min.x, min.y, max.z), color, linewidth);
        DrawLine(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color, linewidth);
        DrawLine(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color, linewidth);
        DrawLine(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color, linewidth);
    }

    void Renderer2D::DrawBox(const AABB& aabb, const glm::vec4& color , float linewidth)
    {
        DrawBox(aabb.min, aabb.max, color, linewidth);
    }

    void Renderer2D::DrawBox(const OBB& obb, const glm::vec4& color , float linewidth)
    {
        for (int i = 0; i < 4; i++) {
            DrawLine(obb.corners[i], obb.corners[(i + 1) % 4], color, linewidth);
            DrawLine(obb.corners[i + 4], obb.corners[(i + 1) % 4 + 4], color, linewidth);
            DrawLine(obb.corners[i], obb.corners[i + 4], color, linewidth);
        }
    }
    
    void Renderer2D::DrawArrow(const glm::vec3& start, const glm::vec3& end, bool fixedLength, glm::vec4 color , float linewidth)
    {
        Batch& batch = GetBatch(RenderMode::World);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::World);
            batch = GetBatch(RenderMode::World);
        }

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        const int arrow_points = 7;
        const float arrow_length = fixedLength ? 1.5f : glm::length(end - start);

        glm::vec3 arrow[arrow_points] = {
            glm::vec3(0, 0, -1),
            glm::vec3(0, 0.8f, 0),
            glm::vec3(0, 0.3f, 0),
            glm::vec3(0, 0.3f, arrow_length),
            glm::vec3(0, -0.3f, arrow_length),
            glm::vec3(0, -0.3f, 0),
            glm::vec3(0, -0.8f, 0)
        };

        const int arrow_sides = 2;

        glm::vec3 direction = glm::normalize(end - start);
        glm::vec3 up = glm::vec3(0, 1, 0);
        if (glm::abs(glm::dot(direction, up)) > 0.99f) {
            up = glm::vec3(1, 0, 0);
        }
        glm::vec3 right = glm::normalize(glm::cross(up, direction));
        up = glm::cross(direction, right);

        glm::mat4 transform = glm::mat4(1.0f);
        transform[0] = glm::vec4(right, 0.0f);
        transform[1] = glm::vec4(up, 0.0f);
        transform[2] = glm::vec4(direction, 0.0f);
        transform[3] = glm::vec4(start, 1.0f);

        for (int i = 0; i < arrow_sides; i++) {
            for (int j = 0; j < arrow_points; j++) {
                if(batch.LineVertices.size() >= Batch::MaxVertices)
                {
                    NextBatch(RenderMode::World);
                    batch = GetBatch(RenderMode::World);
                }

                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::pi<float>() * i / arrow_sides, glm::vec3(0, 0, 1));

                glm::vec3 v1 = arrow[j] - glm::vec3(0, 0, arrow_length);
                glm::vec3 v2 = arrow[(j + 1) % arrow_points] - glm::vec3(0, 0, arrow_length);

                glm::vec3 transformed_v1 = glm::vec3(transform * rotation * glm::vec4(v1, 1.0f));
                glm::vec3 transformed_v2 = glm::vec3(transform * rotation * glm::vec4(v2, 1.0f));

                batch.LineVertices.push_back({transformed_v1, color, entityIDVec3});
                batch.LineVertices.push_back({transformed_v2, color, entityIDVec3});

                
            }
        }
    }
    void Renderer2D::DrawArrow(const glm::vec3& origin, const glm::vec3& direction, float length, glm::vec4 color , float linewidth)
    {
        glm::vec3 end = origin - direction * length;
        DrawArrow(origin, end, false, color, linewidth);
    }

    void Renderer2D::DrawFrustum(const Frustum& frustum, const glm::vec4& color , float linewidth)
    {
        const glm::vec3* points = frustum.GetPoints();
        
        // Draw near plane rectangle
        DrawLine(points[0], points[1], color, linewidth);
        DrawLine(points[0], points[2], color, linewidth);
        DrawLine(points[2], points[3], color, linewidth);
        DrawLine(points[1], points[3], color, linewidth);
        
        // Draw far plane rectangle
        DrawLine(points[4], points[5], color, linewidth);
        DrawLine(points[4], points[6], color, linewidth);
        DrawLine(points[6], points[7], color, linewidth);
        DrawLine(points[5], points[7], color, linewidth);
        
        // Draw connecting lines between near and far planes
        DrawLine(points[0], points[4], color, linewidth);
        DrawLine(points[1], points[5], color, linewidth);
        DrawLine(points[2], points[6], color, linewidth);
        DrawLine(points[3], points[7], color, linewidth);
    }

    void Renderer2D::DrawFrustum(const glm::mat4& viewProjection, const glm::vec4& color , float linewidth)
    {
        glm::mat4 invVP = glm::inverse(viewProjection);

        glm::vec3 frustumCorners[8] = {
            glm::vec3(-1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, -1.0f, -1.0f),
            glm::vec3(1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, 1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(-1.0f, 1.0f, 1.0f)
        };

        for (int i = 0; i < 8; i++) {
            glm::vec4 clipSpace = invVP * glm::vec4(frustumCorners[i], 1.0f);
            clipSpace /= clipSpace.w;
            frustumCorners[i] = clipSpace;
        }

        DrawLine(frustumCorners[0], frustumCorners[1], color, linewidth);
        DrawLine(frustumCorners[1], frustumCorners[2], color, linewidth);
        DrawLine(frustumCorners[2], frustumCorners[3], color, linewidth);
        DrawLine(frustumCorners[3], frustumCorners[0], color, linewidth);

        DrawLine(frustumCorners[4], frustumCorners[5], color, linewidth);
        DrawLine(frustumCorners[5], frustumCorners[6], color, linewidth);
        DrawLine(frustumCorners[6], frustumCorners[7], color, linewidth);
        DrawLine(frustumCorners[7], frustumCorners[4], color, linewidth);

        DrawLine(frustumCorners[0], frustumCorners[4], color, linewidth);
        DrawLine(frustumCorners[1], frustumCorners[5], color, linewidth);
        DrawLine(frustumCorners[2], frustumCorners[6], color, linewidth);
        DrawLine(frustumCorners[3], frustumCorners[7], color, linewidth);
    }

    void Renderer2D::DrawCapsule(const glm::vec3& position, const glm::quat& rotation, float radius, float height, const glm::vec4& color)
    {
        float halfCylinderHeight = height * 0.5f;
        
        glm::vec3 upVector = rotation * glm::vec3(0, 1, 0);
        glm::vec3 topSpherePos = position + upVector * halfCylinderHeight;
        glm::vec3 bottomSpherePos = position - upVector * halfCylinderHeight;
        
        DrawSphere(topSpherePos, radius,glm::identity<glm::quat>(), color);
        DrawSphere(bottomSpherePos, radius,glm::identity<glm::quat>(), color);
        
        DrawCylinder(position, rotation, radius, height, color);
    }
    
    void Renderer2D::DrawCylinder(const glm::vec3& position, const glm::quat& rotation, float radius, float height, const glm::vec4& color)
    {
        const int segments = 24;
        const float angleStep = 2.0f * glm::pi<float>() / segments;
        
        glm::vec3 topPoints[25]; 
        glm::vec3 bottomPoints[25];
        
        float halfHeight = height * 0.5f;
        for (int i = 0; i <= segments; i++)
        {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            glm::vec3 localTop(x, halfHeight, z);
            glm::vec3 localBottom(x, -halfHeight, z);
            
            glm::vec3 worldTop = position + rotation * localTop;
            glm::vec3 worldBottom = position + rotation * localBottom;
            
            topPoints[i] = worldTop;
            bottomPoints[i] = worldBottom;
        }
        
        for (int i = 0; i < segments; i++)
        {
            DrawLine(topPoints[i], topPoints[i + 1], color);
            
            DrawLine(bottomPoints[i], bottomPoints[i + 1], color);
            
            if (i % 4 == 0) {
                DrawLine(topPoints[i], bottomPoints[i], color);
            }
        }
    }

    void Renderer2D::DrawCone(glm::vec3 position, glm::quat rotation, float radius, float height, glm::vec4 color)
    {
        Batch& batch = GetBatch(RenderMode::World);

        if(batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::World);
            batch = GetBatch(RenderMode::World);
        }

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);
    
        // Calculate the apex position (top of the cone)
        glm::vec3 upVector = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 apex = position + upVector * height;
        
        // Draw base circle
        const uint32_t segments = 24;
        const float angleStep = 2.0f * glm::pi<float>() / segments;
        
        glm::vec3 basePoints[25]; // One extra for connecting back to the start
        
        // Calculate points around the base circle
        for (int i = 0; i <= segments; i++)
        {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float z = radius * sin(angle);
            
            glm::vec3 localPoint(x, 0.0f, z);
            glm::vec3 worldPoint = position + rotation * localPoint;
            
            basePoints[i] = worldPoint;
        }
        
        // Draw the base circle
        for (int i = 0; i < segments; i++)
        {
            if(batch.LineVertices.size() >= Batch::MaxVertices)
            {
                NextBatch(RenderMode::World);
                batch = GetBatch(RenderMode::World);
            }
            
            batch.LineVertices.push_back({basePoints[i], color, entityIDVec3});
            batch.LineVertices.push_back({basePoints[i + 1], color, entityIDVec3});
            
        }
        
        // Draw lines from the base to the apex (every few segments for clarity)
        for (int i = 0; i < segments; i += 3)
        {
            if(batch.LineVertices.size() >= Batch::MaxVertices)
            {
                NextBatch(RenderMode::World);
                batch = GetBatch(RenderMode::World);
            }
            
            batch.LineVertices.push_back({basePoints[i], color, entityIDVec3});
            batch.LineVertices.push_back({apex, color, entityIDVec3});
            
        }
    }

    void Renderer2D::DrawTruncatedCone(glm::vec3 position, glm::quat rotation, float baseRadius, float topRadius,
                                       float height, glm::vec4 color)
    {
        Batch& batch = GetBatch(RenderMode::World);

        if (batch.LineVertices.size() >= Batch::MaxVertices)
        {
            NextBatch(RenderMode::World);
            batch = GetBatch(RenderMode::World);
        }

        glm::vec3 entityIDVec3 = glm::vec3(1.0f, 1.0f, 1.0f);

        // Calculate the apex position (top of the cone)
        glm::vec3 upVector = rotation * glm::vec3(0.0f, 1.0f, 0.0f);
        glm::vec3 apex = position + upVector * height;

        // Draw base circle
        const uint32_t segments = 24;
        const float angleStep = 2.0f * glm::pi<float>() / segments;

        glm::vec3 basePoints[25]; // One extra for connecting back to the start
        glm::vec3 topPoints[25];  // One extra for connecting back to the start

        // Calculate points around the base circle
        for (int i = 0; i <= segments; i++)
        {
            float angle = i * angleStep;

            // Base circle
            float baseX = baseRadius * cos(angle);
            float baseZ = baseRadius * sin(angle);
            glm::vec3 baseLocalPoint(baseX, 0.0f, baseZ);
            glm::vec3 baseWorldPoint = position + rotation * baseLocalPoint;
            basePoints[i] = baseWorldPoint;

            // Top circle
            float topX = topRadius * cos(angle);
            float topZ = topRadius * sin(angle);
            glm::vec3 topLocalPoint(topX, height, topZ);
            glm::vec3 topWorldPoint = position + rotation * topLocalPoint;
            topPoints[i] = topWorldPoint;
        }

        // Draw the base circle
        for (int i = 0; i < segments; i++)
        {
            if (batch.LineVertices.size() >= Batch::MaxVertices)
            {
                NextBatch(RenderMode::World);
                batch = GetBatch(RenderMode::World);
            }

            batch.LineVertices.push_back({basePoints[i], color, entityIDVec3});
            batch.LineVertices.push_back({basePoints[i + 1], color, entityIDVec3});
        }

        // Draw the top circle
        for (int i = 0; i < segments; i++)
        {
            if (batch.LineVertices.size() >= Batch::MaxVertices)
            {
                NextBatch(RenderMode::World);
                batch = GetBatch(RenderMode::World);
            }

            batch.LineVertices.push_back({topPoints[i], color, entityIDVec3});
            batch.LineVertices.push_back({topPoints[i + 1], color, entityIDVec3});
        }

        // Draw lines from the base to the top (side edges of the truncated cone)
        for (int i = 0; i < segments; i++)
        {
            if (batch.LineVertices.size() >= Batch::MaxVertices)
            {
                NextBatch(RenderMode::World);
                batch = GetBatch(RenderMode::World);
            }

            batch.LineVertices.push_back({basePoints[i], color, entityIDVec3});
            batch.LineVertices.push_back({topPoints[i], color, entityIDVec3});
        }
    }


    void Renderer2D::DrawTextString(const std::string &text, Ref<Font> font, const glm::mat4 &transform, const TextParams &textParams, RenderMode mode, uint32_t entityID)
    {

        Batch& batch = GetBatch(mode);

        if(batch.TextIndexCount >= Batch::MaxIndices)
        {
            NextBatch(mode);
            batch = GetBatch(mode);
        }

        const auto& fontGeometry = font->GetMSDFData()->FontGeometry;
        const auto& metrics = fontGeometry.getMetrics();
        Ref<Texture2D> fontAtlas = font->GetAtlasTexture();
        
        // TODO: Skip to the next batch if the font is different
        batch.FontAtlasTexture = fontAtlas;

        double fsScale = textParams.Size / (metrics.ascenderY - metrics.descenderY);
        const float spaceGlyphAdvance = fontGeometry.getGlyph(' ')->getAdvance();

        std::vector<std::string> lines;
        std::string currentLine;

        for (size_t i = 0; i < text.size(); i++) {
            if (text[i] == '\n') {
                lines.push_back(currentLine);
                currentLine.clear();
            } else if (text[i] != '\r') {
                currentLine += text[i];
            }
        }

        if (!currentLine.empty())
            lines.push_back(currentLine);

        if (lines.empty())
            lines.push_back("");

		double y = 0.0;

		for (const auto& line : lines) {
            double lineWidth = 0.0;

            if (textParams.Alignment != TextAlignment::Left) {
                for (size_t i = 0; i < line.size(); i++) {
                    char character = line[i];

                    if (character == ' ') {
                        lineWidth += fsScale * spaceGlyphAdvance + textParams.Kerning;
                        continue;
                    }

                    if (character == '\t') {
                        lineWidth += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
                        continue;
                    }

                    auto glyph = fontGeometry.getGlyph(character);
                    if (!glyph)
                        glyph = fontGeometry.getGlyph('?');
                    if (!glyph)
                        continue;

                    double advance = glyph->getAdvance();
                    if (i < line.size() - 1) {
                        char nextCharacter = line[i + 1];
                        fontGeometry.getAdvance(advance, character, nextCharacter);
                    }

                    lineWidth += fsScale * advance + textParams.Kerning;
                }

                if (!line.empty())
                    lineWidth -= textParams.Kerning;
            }

            double x = 0.0;
            if (textParams.Alignment == TextAlignment::Center)
                x = -lineWidth / 2.0;
            else if (textParams.Alignment == TextAlignment::Right)
                x = -lineWidth;

            for (size_t i = 0; i < line.size(); i++) {
                char character = line[i];

                if (character == ' ') {
                    float advance = spaceGlyphAdvance;
                    if (i < line.size() - 1) {
                        char nextCharacter = line[i + 1];
                        double dAdvance;
                        fontGeometry.getAdvance(dAdvance, character, nextCharacter);
                        advance = (float)dAdvance;
                    }

                    x += fsScale * advance + textParams.Kerning;
                    continue;
                }

                if (character == '\t') {
                    x += 4.0f * (fsScale * spaceGlyphAdvance + textParams.Kerning);
                    continue;
                }

                auto glyph = fontGeometry.getGlyph(character);
                if (!glyph)
                    glyph = fontGeometry.getGlyph('?');
                if (!glyph)
                    continue;

                double al, ab, ar, at;
                glyph->getQuadAtlasBounds(al, ab, ar, at);
                glm::vec2 texCoordMin((float)al, (float)ab);
                glm::vec2 texCoordMax((float)ar, (float)at);

                double pl, pb, pr, pt;
                glyph->getQuadPlaneBounds(pl, pb, pr, pt);
                glm::vec2 quadMin((float)pl, (float)pb);
                glm::vec2 quadMax((float)pr, (float)pt);

                quadMin *= fsScale, quadMax *= fsScale;
                quadMin += glm::vec2(x, y);
                quadMax += glm::vec2(x, y);

                float texelWidth = 1.0f / fontAtlas->GetWidth();
                float texelHeight = 1.0f / fontAtlas->GetHeight();
                texCoordMin *= glm::vec2(texelWidth, texelHeight);
                texCoordMax *= glm::vec2(texelWidth, texelHeight);

                // Convert entityID to vec3
                uint32_t r = (entityID & 0x000000FF) >> 0;
                uint32_t g = (entityID & 0x0000FF00) >> 8;
                uint32_t b = (entityID & 0x00FF0000) >> 16;
                glm::vec3 entityIDVec3 = glm::vec3(r / 255.0f, g / 255.0f, b / 255.0f);

                batch.TextVertices.push_back({
                    transform * glm::vec4(quadMin, 0.0f, 1.0f),
                    textParams.Color,
                    texCoordMin,
                    entityIDVec3
                });

                batch.TextVertices.push_back({
                    transform * glm::vec4(quadMin.x, quadMax.y, 0.0f, 1.0f),
                    textParams.Color,
                    { texCoordMin.x, texCoordMax.y },
                    entityIDVec3
                });

                batch.TextVertices.push_back({
                    transform * glm::vec4(quadMax, 0.0f, 1.0f),
                    textParams.Color,
                    texCoordMax,
                    entityIDVec3
                });

                batch.TextVertices.push_back({
                    transform * glm::vec4(quadMax.x, quadMin.y, 0.0f, 1.0f),
                    textParams.Color,
                    { texCoordMax.x, texCoordMin.y },
                    entityIDVec3
                });

                batch.TextIndexCount += 6;

                if (i < line.size() - 1) {
                    double advance = glyph->getAdvance();
                    char nextCharacter = line[i + 1];
                    fontGeometry.getAdvance(advance, character, nextCharacter);

                    x += fsScale * advance + textParams.Kerning;
                }
            }

            y -= fsScale * metrics.lineHeight + textParams.LineSpacing;
        }
    }

    Batch& Renderer2D::GetBatch(RenderMode mode)
    {
        auto& batches = (mode == RenderMode::World) ? s_Renderer2DData.WorldBatches : s_Renderer2DData.ScreenBatches;
        if (batches.empty())
        {
            batches.emplace();
        }
        return batches.back();
    }
    
    void Renderer2D::NextBatch(RenderMode mode)
    {
        auto& batches = (mode == RenderMode::World) ? s_Renderer2DData.WorldBatches : s_Renderer2DData.ScreenBatches;
        batches.emplace();
    }
}