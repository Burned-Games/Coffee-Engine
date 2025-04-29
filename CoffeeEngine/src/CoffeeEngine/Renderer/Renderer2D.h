#pragma once

#include "CoffeeEngine/Renderer/Font.h"
#include "CoffeeEngine/Renderer/RenderTarget.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>


namespace Coffee {

    struct Batch;
    struct AABB;
    struct OBB;

    class Frustum;
    
    class Renderer2D
    {
    public:
        enum class RenderMode
        {
            World,
            Screen
        };
    public:
        static void Init();

        /*
        static Render();

        or

        static RenderWorld();
        static RenderUI();

        or

        static WorldPass();
        static UIPass();
        */

        static void WorldPass(const RenderTarget& target);
        static void ScreenPass(const RenderTarget& target);

        static void Shutdown();

        static void DrawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f), uint32_t entityID = 4294967295);
        static void DrawRect(const glm::vec2& position, const glm::vec2& size, const float rotation, const glm::vec4& color = glm::vec4(1.0f), uint32_t entityID = 4294967295);
        // Billboard?
        static void DrawRect(const glm::vec3& position, const glm::vec2& size, const glm::vec4& color = glm::vec4(1.0f), uint32_t entityID = 4294967295);
        static void DrawRect(const glm::mat4& transform, const glm::vec4& color, RenderMode mode, uint32_t entityID = 4294967295);

        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::vec2& position, const glm::vec2& size, const float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::vec3& position, const glm::vec2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const glm::vec4& tintColor = glm::vec4(1.0f));
        static void DrawQuad(const glm::mat4& transform, const Ref<Texture2D>& texture, float tilingFactor, const glm::vec4& tintColor, RenderMode mode, uint32_t entityID = 4294967295, const glm::vec4& uvRect = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));

        static void DrawLine(const glm::vec2& start, const glm::vec2& end, const glm::vec4& color = glm::vec4(1.0f), float linewidth = 1.0f);
        static void DrawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color = glm::vec4(1.0f), float linewidth = 1.0f);

        static void DrawCircle(const glm::vec2& position, float radius, const glm::vec4& color = glm::vec4(1.0f), float linewidth = 1.0f);
        static void DrawCircle(const glm::vec3& position, float radius, const glm::quat& rotation = glm::identity<glm::quat>(), const glm::vec4& color = glm::vec4(1.0f), float linewidth = 1.0f);

        static void DrawSphere(const glm::vec3& position, float radius, const glm::quat& rotation = glm::identity<glm::quat>(), const glm::vec4& color = glm::vec4(1.0f), float linewidth = 1.0f);

        static void DrawBox(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& size, const glm::vec4& color = glm::vec4(1.0f), const bool& isCentered = true, float linewidth = 1.0f);
        static void DrawBox(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color = glm::vec4(1.0f), float lineWidth = 1.0f);
        static void DrawBox(const AABB& aabb, const glm::vec4& color = glm::vec4(1.0f), float lineWidth = 1.0f);
        static void DrawBox(const OBB& obb, const glm::vec4& color = glm::vec4(1.0f), float lineWidth = 1.0f);
        
        static void DrawArrow(const glm::vec3& start, const glm::vec3& end, bool fixedLength, glm::vec4 color = glm::vec4(1.0f), float lineWidth = 1.0f);
        static void DrawArrow(const glm::vec3& origin, const glm::vec3& direction, float length, glm::vec4 color = glm::vec4(1.0f), float lineWidth = 1.0f);

        static void DrawFrustum(const Frustum& frustum, const glm::vec4& color = glm::vec4(1.0f), float lineWidth = 1.0f);
        static void DrawFrustum(const glm::mat4& viewProjection, const glm::vec4& color = glm::vec4(1.0f), float lineWidth = 1.0f);

        static void DrawCapsule(const glm::vec3& position, const glm::quat& rotation, float radius, float height, const glm::vec4& color = glm::vec4(1.0f));
        
        static void DrawCylinder(const glm::vec3& position, const glm::quat& rotation, float radius, float height,
                                 const glm::vec4& color = glm::vec4(1.0f));

        static void DrawCone(glm::vec3 vec, glm::quat qua, float radius, float height, glm::vec4 vec4);

        struct TextParams
		{
			glm::vec4 Color{ 1.0f };
			float Kerning = 0.0f;
			float LineSpacing = 0.0f;
            float Size = 16.0f;
		};

        static void DrawTextString(const std::string& text, Ref<Font> font, const glm::mat4& transform, const TextParams& textParams, RenderMode mode, uint32_t entityID = 4294967295);
    private:
        static Batch& GetBatch(RenderMode mode);
        static void NextBatch(RenderMode mode);
    };

}