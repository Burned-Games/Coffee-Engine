#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/IO/ImportData/Texture2DImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Embedded/EquirectToCubemap.inl"
#include "CoffeeEngine/Embedded/IrradianceConvolution.inl"
#include "CoffeeEngine/Embedded/PreFilterConvolutionShader.inl"
#include "CoffeeEngine/Embedded/BRDFLUTShader.inl"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <stb_image.h>
#include <glm/vec4.hpp>
#include <tracy/Tracy.hpp>

namespace Coffee {

    GLenum ImageFormatToOpenGLInternalFormat(ImageFormat format)
    {
        switch(format)
        {
            case ImageFormat::R8: return GL_R8; break;
            case ImageFormat::RG8: return GL_RG8; break;
            case ImageFormat::RGB8: return GL_RGB8; break;
            case ImageFormat::SRGB8: return GL_SRGB8; break;
            case ImageFormat::RGBA8: return GL_RGBA8; break;
            case ImageFormat::SRGBA8: return GL_SRGB8_ALPHA8; break;
            case ImageFormat::R16F: return GL_R16F; break;
            case ImageFormat::RG16F: return GL_RG16F; break;
            case ImageFormat::RGB16F: return GL_RGB16F; break;
            case ImageFormat::RGBA16F: return GL_RGBA16F; break;
            case ImageFormat::R32F: return GL_R32F; break;
            case ImageFormat::RGB32F: return GL_RGB32F; break;
            case ImageFormat::RGBA32F: return GL_RGBA32F; break;
            case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH24_STENCIL8; break;
        }
    }

    GLenum ImageFormatToOpenGLFormat(ImageFormat format)
    {
        switch(format)
        {
            case ImageFormat::R8: return GL_RED; break;
            case ImageFormat::RG8: return GL_RG; break;
            case ImageFormat::RGB8: return GL_RGB; break;
            case ImageFormat::SRGB8: return GL_RGB; break;
            case ImageFormat::RGBA8: return GL_RGBA; break;
            case ImageFormat::SRGBA8: return GL_RGBA; break;
            case ImageFormat::R16F: return GL_RED; break;
            case ImageFormat::RG16F: return GL_RG; break;
            case ImageFormat::RGB16F: return GL_RGB; break;
            case ImageFormat::RGBA16F: return GL_RGBA; break;
            case ImageFormat::R32F: return GL_RED; break;
            case ImageFormat::RGB32F: return GL_RGB; break;
            case ImageFormat::RGBA32F: return GL_RGBA; break;
            case ImageFormat::DEPTH24STENCIL8: return GL_DEPTH_STENCIL; break;
        }
    }

    int ImageFormatToChannelCount(ImageFormat format)
    {
        switch(format)
        {
            case ImageFormat::R8: return 1; break;
            case ImageFormat::RG8: return 2; break;
            case ImageFormat::RGB8: return 3; break;
            case ImageFormat::SRGB8: return 3; break;
            case ImageFormat::RGBA8: return 4; break;
            case ImageFormat::SRGBA8: return 4; break;
            case ImageFormat::R16F: return 1; break;
            case ImageFormat::RG16F: return 2; break;
            case ImageFormat::RGB16F: return 3; break;
            case ImageFormat::RGBA16F: return 4; break;
            case ImageFormat::R32F: return 1; break;
            case ImageFormat::RGB32F: return 3; break;
            case ImageFormat::RGBA32F: return 4; break;
            case ImageFormat::DEPTH24STENCIL8: return 1; break;
        }
    }
    
    int TextureWrapToOpenGL(TextureWrap wrap)
    {
        switch(wrap)
        {
            case TextureWrap::Repeat: return GL_REPEAT; break;
            case TextureWrap::MirroredRepeat: return GL_MIRRORED_REPEAT; break;
            case TextureWrap::ClampToEdge: return GL_CLAMP_TO_EDGE; break;
            case TextureWrap::ClampToBorder: return GL_CLAMP_TO_BORDER; break;
        }
    }

    int TextureFilterToOpenGL(TextureFilter filter)
    {
        switch (filter)
        {
            case TextureFilter::Nearest: return GL_NEAREST; break;
            case TextureFilter::Linear: return GL_LINEAR; break;
            case TextureFilter::NearestMipmapNearest: return GL_NEAREST_MIPMAP_NEAREST; break;
            case TextureFilter::LinearMipmapNearest: return GL_LINEAR_MIPMAP_NEAREST; break;
            case TextureFilter::NearestMipmapLinear: return GL_NEAREST_MIPMAP_LINEAR; break;
            case TextureFilter::LinearMipmapLinear: return GL_LINEAR_MIPMAP_LINEAR; break;
        }
    }

    Texture2D::Texture2D(const TextureProperties& properties)
        : m_Properties(properties), m_Width(properties.Width), m_Height(properties.Height)
    {
        ZoneScoped;

        InitializeTexture2D();
    }

    Texture2D::Texture2D(uint32_t width, uint32_t height, ImageFormat imageFormat, TextureWrap wrapping,
                         TextureFilter minFilter, TextureFilter magFilter, TextureFilter mipMapFilter, const glm::vec4& borderColor)
        : Texture(ResourceType::Texture2D), m_Width(width), m_Height(height),
          m_Properties({imageFormat, wrapping, minFilter, magFilter, mipMapFilter, borderColor})
    {
        ZoneScoped;

        InitializeTexture2D();
    }

    Texture2D::Texture2D(const std::filesystem::path& path, bool srgb)
        : Texture(ResourceType::Texture2D)
    {
        ZoneScoped;

        m_FilePath = path;
        m_Name = path.filename().string();

        m_Properties.srgb = srgb;

        LoadFromFile(path);
    }

    Texture2D::Texture2D(ImportData& importData)
        : Texture(ResourceType::Texture2D)
    {
        Texture2DImportData& texture2DImportData = dynamic_cast<Texture2DImportData&>(importData);

        if(texture2DImportData.IsValid())
        {
            m_FilePath = texture2DImportData.originalPath;
            m_Name = m_FilePath.filename().string();
            m_Properties.srgb = texture2DImportData.sRGB;

            LoadFromFile(m_FilePath);
    
            m_UUID = texture2DImportData.uuid;
        }
        else
        {
            m_FilePath = texture2DImportData.originalPath;
            m_Name = m_FilePath.filename().string();
            m_Properties.srgb = texture2DImportData.sRGB;

            LoadFromFile(m_FilePath);

            texture2DImportData.uuid = m_UUID;
        }
    }

    Texture2D::~Texture2D()
    {
        ZoneScoped;

        glDeleteTextures(1, &m_textureID);

        if(m_Data.size() > 0)
        {
            m_Data.clear();
        }
    }

    void Texture2D::Bind(uint32_t slot)
    {
        ZoneScoped;

        glBindTextureUnit(slot, m_textureID);
    }

    void Texture2D::Resize(uint32_t width, uint32_t height)
    {
        ZoneScoped;

        m_Width = width;
        m_Height = height;

        glDeleteTextures(1, &m_textureID);

        int mipLevels = 1 + floor(log2(std::max(m_Width, m_Height)));

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
        glTextureStorage2D(m_textureID, mipLevels, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //Add an option to choose the anisotropic filtering level
        glTextureParameterf(m_textureID, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);

        //Te code above is the same as the constructor but for some reason it doesn't work
        //Texture2D(m_Width, m_Height, m_Properties.Format);
    }

    void Texture2D::Clear(glm::vec4 color)
    {
        ZoneScoped;

        glBindTexture(GL_TEXTURE_2D, m_textureID);

        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);
        glClearTexImage(m_textureID, 0, format, GL_FLOAT, &color);
    }

    void Texture2D::SetData(void* data, uint32_t size)
    {
        ZoneScoped;

        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);
        //COFFEE_ASSERT(size == m_Width * m_Height * ImageFormatToChannelCount(m_Properties.Format), "Data must be entire texture!");
        glTextureSubImage2D(m_textureID, 0, 0, 0, m_Width, m_Height, format, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(m_textureID);
    }

    Ref<Texture2D> Texture2D::Load(const std::filesystem::path& path)
    {
        return ResourceLoader::Load<Texture2D>(path);
    }

    Ref<Texture2D> Texture2D::Create(uint32_t width, uint32_t height, ImageFormat format)
    {
        return CreateRef<Texture2D>(width, height, format);
    }

    Ref<Texture2D> Texture2D::Create(const TextureProperties& properties)
    {
        return CreateRef<Texture2D>(properties);
    }

    void Texture2D::LoadFromFile(const std::filesystem::path& path)
    {
        int nrComponents;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(m_FilePath.string().c_str(), &m_Width, &m_Height, &nrComponents, 0);

        m_Properties.Width = m_Width, m_Properties.Height = m_Height;

        if(data)
        {
            m_Data = std::vector<unsigned char>(data, data + m_Width * m_Height * nrComponents);
            stbi_image_free(data);

            switch (nrComponents)
            {
                case 1:
                    m_Properties.Format = ImageFormat::R8;
                break;
                case 3:
                    m_Properties.Format = m_Properties.srgb ? ImageFormat::SRGB8 : ImageFormat::RGB8;
                break;
                case 4:
                    m_Properties.Format = m_Properties.srgb ? ImageFormat::SRGBA8 : ImageFormat::RGBA8;
                break;
            }

            InitializeTexture2D();
            SetData(m_Data.data(), m_Data.size());
        }
        else
        {
            COFFEE_CORE_ERROR("Failed to load texture: {0} (REASON: {1})", m_FilePath.string(), stbi_failure_reason());
            m_textureID = 0; // Set texture ID to 0 to indicate failure
        }
    }

    void Texture2D::InitializeTexture2D()
    {
        int mipLevels = m_Properties.GenerateMipmaps ? 1 + floor(log2(std::max(m_Width, m_Height))) : 1;

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
        glTextureStorage2D(m_textureID, mipLevels, internalFormat, m_Width, m_Height);

        GLenum wrap = TextureWrapToOpenGL(m_Properties.Wrapping);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, wrap);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, wrap);

        glTextureParameterfv(m_textureID, GL_TEXTURE_BORDER_COLOR, &m_Properties.BorderColor[0]);

        GLenum minFilter = TextureFilterToOpenGL(m_Properties.MinFilter);
        GLenum magFilter = TextureFilterToOpenGL(m_Properties.MagFilter);
        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, minFilter);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, magFilter);

        //Add an option to choose the anisotropic filtering level
        glTextureParameterf(m_textureID, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
    }

    static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    static glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

    Ref<Mesh> Cubemap::m_CubeMesh;

    Cubemap::Cubemap()
        : Texture(ResourceType::Cubemap)
    {
        ZoneScoped;

        m_Properties.srgb = false;
        m_Properties.Format = ImageFormat::RGB32F;
    }

    Cubemap::Cubemap(const std::filesystem::path& path) : Cubemap()
    {
        ZoneScoped;

        LoadFromFile(path);
    }

    Cubemap::Cubemap(ImportData& importData) : Cubemap()
    {
        if(importData.IsValid())
        {
            LoadFromFile(importData.originalPath);
            m_UUID = importData.uuid;
        }
        else
        {
            LoadFromFile(importData.originalPath);
            importData.uuid = m_UUID;
        }
    }

    Cubemap::~Cubemap()
    {
        ZoneScoped;
        glDeleteTextures(1, &m_CubeMapID);
        glDeleteTextures(1, &m_IrradianceMapID);
    }

    void Cubemap::Bind(uint32_t slot)
    {
        glBindTextureUnit(slot, m_CubeMapID);
    }

    void Cubemap::BindIrradianceMap(uint32_t slot)
    {
        glBindTextureUnit(slot, m_IrradianceMapID);
    }

    void Cubemap::BindPrefilteredMap(uint32_t slot)
    {
        glBindTextureUnit(slot, m_PrefilteredMapID);
    }

    void Cubemap::LoadFromFile(const std::filesystem::path& path)
    {
        m_FilePath = path;
        m_Name = path.filename().string();

        m_Properties.srgb = false;

        LoadEquirectangularMapFromFile(path);
        GenerateIrradianceMap();
        GeneratePrefilteredMap();
    }

    void Cubemap::LoadEquirectangularMapFromFile(const std::filesystem::path& path)
    {
        int nrChannels;
        int width, height;
        stbi_set_flip_vertically_on_load(true);
        float* data = stbi_loadf(path.string().c_str(), &width, &height, &nrChannels, 0);
        if (!data) {
            COFFEE_CORE_ERROR("Failed to load cubemap texture: {0} (REASON: {1})", m_FilePath.string(), stbi_failure_reason());
            return;
        }

        switch (nrChannels)
        {
            case 1:
                m_Properties.Format = ImageFormat::R32F;
                break;
            case 3:
                m_Properties.Format = ImageFormat::RGB32F;
                break;
            case 4:
                m_Properties.Format = ImageFormat::RGBA32F;
                break;
        }

        EquirectToCubemap(data, width, height);

        stbi_image_free(data);
    }

    void Cubemap::EquirectToCubemap(float* data, int width, int height)
    {
        // Load the equirectangular image to the GPU
        uint32_t equirectTextureID;
        glCreateTextures(GL_TEXTURE_2D, 1, &equirectTextureID);
        glTextureParameteri(equirectTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(equirectTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(equirectTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(equirectTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureStorage2D(equirectTextureID, 1, GL_RGB32F, width, height);
        glTextureSubImage2D(equirectTextureID, 0, 0, 0, width, height, GL_RGB, GL_FLOAT, data);
        
        // Resolution of the cubemap faces
        int cubemapFaceSize = width / 4;

        m_Properties.Width = cubemapFaceSize;
        m_Properties.Height = cubemapFaceSize;

        glCreateFramebuffers(1, &fbo);
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, cubemapFaceSize, cubemapFaceSize);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        int mipLevels = 1 + floor(log2(std::max(cubemapFaceSize, cubemapFaceSize)));

        // Create the cubemap texture
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubeMapID);
        glTextureStorage2D(m_CubeMapID, mipLevels, GL_RGB32F, cubemapFaceSize, cubemapFaceSize);

        glTextureParameteri(m_CubeMapID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_CubeMapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        static Ref<Shader> shader = CreateRef<Shader>("EquirectangularToCubemap", equirectToCubemapSource);
        m_CubeMesh = PrimitiveMesh::CreateCube({-1.0f, -1.0f, -1.0f});

        shader->Bind();
        shader->setMat4("projection", captureProjection);
        shader->setInt("equirectangularMap", 0);
        glBindTextureUnit(0, equirectTextureID);

        glViewport(0, 0, cubemapFaceSize, cubemapFaceSize);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        for (uint32_t i = 0; i < 6; ++i)
        {
            shader->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_CubeMapID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_CubeMesh->GetVertexArray()->Bind();
            glDrawElements(GL_TRIANGLES, m_CubeMesh->GetVertexArray()->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        }

        glGenerateTextureMipmap(m_CubeMapID);

        glDeleteTextures(1, &equirectTextureID);
    }

    void Cubemap::GenerateIrradianceMap()
    {
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_IrradianceMapID);
        glTextureStorage2D(m_IrradianceMapID, 1, GL_RGB32F, m_IrradianceMapResolution, m_IrradianceMapResolution);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, m_IrradianceMapResolution, m_IrradianceMapResolution);

        static Ref<Shader> irradianceShader = CreateRef<Shader>("IrradianceConvolution", irradianceConvolutionSource);

        irradianceShader->Bind();
        irradianceShader->setInt("environmentMap", 0);
        irradianceShader->setMat4("projection", captureProjection);
        glBindTextureUnit(0, m_CubeMapID);

        glViewport(0, 0, m_IrradianceMapResolution, m_IrradianceMapResolution);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        for (uint32_t i = 0; i < 6; ++i)
        {
            irradianceShader->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_IrradianceMapID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            m_CubeMesh->GetVertexArray()->Bind();
            glDrawElements(GL_TRIANGLES, m_CubeMesh->GetVertexArray()->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        }
    }

    void Cubemap::GeneratePrefilteredMap()
    {
        unsigned int maxMipLevels = 5;

        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PrefilteredMapID);
        glTextureStorage2D(m_PrefilteredMapID, maxMipLevels, GL_RGB32F, m_PrefilteredMapResolution, m_PrefilteredMapResolution);
        glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, m_PrefilteredMapResolution, m_PrefilteredMapResolution);

        static Ref<Shader> prefilterShader = CreateRef<Shader>("PreFilterConvolution", PreFilterConvolutionSource);

        prefilterShader->Bind();
        prefilterShader->setInt("environmentMap", 0);
        prefilterShader->setMat4("projection", captureProjection);
        glBindTextureUnit(0, m_CubeMapID);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        for (unsigned int mip = 0; mip < maxMipLevels; mip++)
        {
            // Resize framebuffer according to mip-level size.
            unsigned int mipWidth = m_PrefilteredMapResolution * std::pow(0.5, mip);
            unsigned int mipHeight = m_PrefilteredMapResolution * std::pow(0.5, mip);
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader->setFloat("roughness", roughness);
            for (uint32_t i = 0; i < 6; ++i)
            {
                prefilterShader->setMat4("view", captureViews[i]);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_PrefilteredMapID, mip);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                m_CubeMesh->GetVertexArray()->Bind();
                glDrawElements(GL_TRIANGLES, m_CubeMesh->GetVertexArray()->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &fbo);
        glDeleteRenderbuffers(1, &rbo);
    }

    Ref<Cubemap> Cubemap::Load(const std::filesystem::path& path)
    {
        return ResourceLoader::Load<Cubemap>(path);
    }
    Ref<Cubemap> Cubemap::Create(const std::filesystem::path& path)
    {
        return CreateRef<Cubemap>(path);
    }

} // namespace Coffee
