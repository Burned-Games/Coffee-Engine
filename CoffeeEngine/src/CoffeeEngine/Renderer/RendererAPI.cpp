#include "CoffeeEngine/Renderer/RendererAPI.h"

#include <cstdint>
#include <glad/glad.h>
#include <sys/types.h>
#include <tracy/Tracy.hpp>

namespace Coffee {

	Scope<RendererAPI> RendererAPI::s_RendererAPI = RendererAPI::Create();

    void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
			case GL_DEBUG_SEVERITY_HIGH:         COFFEE_CORE_ERROR(message); return;
			case GL_DEBUG_SEVERITY_MEDIUM:       COFFEE_CORE_ERROR(message); return;
			case GL_DEBUG_SEVERITY_LOW:          COFFEE_CORE_WARN(message); return;
			case GL_DEBUG_SEVERITY_NOTIFICATION: COFFEE_CORE_TRACE(message); return;
		}

		COFFEE_CORE_ASSERT(false, "Unknown severity level!");
	}

    void RendererAPI::Init()
    {
        ZoneScoped;

	#ifdef COFFEE_DEBUG
			glEnable(GL_DEBUG_OUTPUT);
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);//can slow down the program
			glDebugMessageCallback(OpenGLMessageCallback, nullptr);

			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
	#endif

        glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		glDepthFunc(GL_LEQUAL);

		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    }

	void RendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		ZoneScoped;

		glViewport(x, y, width, height);
	}

	void RendererAPI::SetClearColor(const glm::vec4& color)
	{
	    ZoneScoped;

		glClearColor(color.r, color.g, color.b, color.a);
	}

	void RendererAPI::Clear(uint32_t clearFlags)
	{
		ZoneScoped;

		uint32_t mask = 0;

		if (clearFlags & (uint32_t)ClearFlags::Color)
		{
			mask |= GL_COLOR_BUFFER_BIT;
		}
		if (clearFlags & (uint32_t)ClearFlags::Depth)
		{
			mask |= GL_DEPTH_BUFFER_BIT;
		}
		if (clearFlags & (uint32_t)ClearFlags::Stencil)
		{
			mask |= GL_STENCIL_BUFFER_BIT;
		}
		if (mask == 0)
		{
			return;
		}
		glClear(mask);
	}

	void RendererAPI::SetColorMask(bool red, bool green, bool blue, bool alpha)
	{
		ZoneScoped;

		glColorMask(red, green, blue, alpha);
	}

	void RendererAPI::SetDepthMask(bool enabled)
	{
		ZoneScoped;

		glDepthMask(enabled);
	}

	void RendererAPI::SetDepthFunc(DepthFunc func)
	{
		ZoneScoped;

		switch (func)
		{
			case DepthFunc::Never:        glDepthFunc(GL_NEVER); break;
			case DepthFunc::Less:         glDepthFunc(GL_LESS); break;
			case DepthFunc::Equal:        glDepthFunc(GL_EQUAL); break;
			case DepthFunc::LessEqual:    glDepthFunc(GL_LEQUAL); break;
			case DepthFunc::Greater:      glDepthFunc(GL_GREATER); break;
			case DepthFunc::NotEqual:     glDepthFunc(GL_NOTEQUAL); break;
			case DepthFunc::GreaterEqual: glDepthFunc(GL_GEQUAL); break;
			case DepthFunc::Always:       glDepthFunc(GL_ALWAYS); break;
			default: COFFEE_CORE_ASSERT(false, "Unknown depth function!"); break;
		}
	}

	void RendererAPI::SetBlend(bool enabled)
	{
		ZoneScoped;

		if (enabled)
		{
			glEnable(GL_BLEND);
		}
		else
		{
			glDisable(GL_BLEND);
		}
	}

	void RendererAPI::SetBlendFunc(BlendFunc src, BlendFunc dst)
	{
		ZoneScoped;

		glBlendFunc(
			static_cast<GLenum>(src),
			static_cast<GLenum>(dst)
		);
	}

	void RendererAPI::SetBlendEquation(BlendEquation equation)
	{
		ZoneScoped;

		glBlendEquation(
			static_cast<GLenum>(equation)
		);
	}

	void RendererAPI::SetFaceCulling(bool enabled)
	{
		ZoneScoped;

		if(enabled)
		{
			glEnable(GL_CULL_FACE);
		}
		else
		{
			glDisable(GL_CULL_FACE);
		}
	}

	void RendererAPI::SetCullFace(CullFace face)
	{
		ZoneScoped;

		switch (face)
		{
			case CullFace::Front:         glCullFace(GL_FRONT); break;
			case CullFace::Back:          glCullFace(GL_BACK); break;
			case CullFace::FrontAndBack:  glCullFace(GL_FRONT_AND_BACK); break;
			default: COFFEE_CORE_ASSERT(false, "Unknown cull face!"); break;
		}
	}

	void RendererAPI::SetPolygonMode(PolygonMode mode)
	{
		ZoneScoped;

		switch (mode)
		{
			case PolygonMode::Fill: glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); break;
			case PolygonMode::Line: glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); break;
			case PolygonMode::Point: glPolygonMode(GL_FRONT_AND_BACK, GL_POINT); break;
			default: COFFEE_CORE_ASSERT(false, "Unknown polygon mode!"); break;
		}
	}

    void RendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
    {
        ZoneScoped;

        vertexArray->Bind();
		vertexArray->GetVertexBuffers()[0]->Bind();

		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
		vertexArray->GetIndexBuffer()->Bind();
		
        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    }

	void RendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount, float lineWidth)
	{
		ZoneScoped;

		vertexArray->Bind();
		glLineWidth(lineWidth);
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

    Scope<RendererAPI> RendererAPI::Create()
    {
        return CreateScope<RendererAPI>();
    }

}
