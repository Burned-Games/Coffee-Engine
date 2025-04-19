#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/IO/ImportData/Texture2DImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Embedded/EquirectToCubemap.inl"
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
            case ImageFormat::R32F: return 1; break;
            case ImageFormat::RGB32F: return 3; break;
            case ImageFormat::RGBA32F: return 4; break;
            case ImageFormat::DEPTH24STENCIL8: return 1; break;
        }
    }

    Texture2D::Texture2D(const TextureProperties& properties)
        : m_Properties(properties), m_Width(properties.Width), m_Height(properties.Height)
    {
        ZoneScoped;

        InitializeTexture2D();
    }

    Texture2D::Texture2D(uint32_t width, uint32_t height, ImageFormat imageFormat)
        : Texture(ResourceType::Texture2D), m_Width(width), m_Height(height), m_Properties({ imageFormat, width, height })
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
        int mipLevels = 1 + floor(log2(std::max(m_Width, m_Height)));

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        glCreateTextures(GL_TEXTURE_2D, 1, &m_textureID);
        glTextureStorage2D(m_textureID, mipLevels, internalFormat, m_Width, m_Height);

        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        //Add an option to choose the anisotropic filtering level
        glTextureParameterf(m_textureID, GL_TEXTURE_MAX_ANISOTROPY, 16.0f);
    }

    Cubemap::Cubemap(const std::vector<std::filesystem::path>& paths) : Texture(ResourceType::Cubemap)
    {
        ZoneScoped;
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < paths.size(); i++)
        {
            unsigned char* data = stbi_load(paths[i].string().c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                GLenum format;
                if (nrChannels == 1)
                    format = GL_RED;
                else if (nrChannels == 3)
                    format = GL_RGB;
                else if (nrChannels == 4)
                    format = GL_RGBA;

                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                COFFEE_CORE_ERROR("Cubemap texture failed to load at path: {0}", paths[i].string());
                stbi_image_free(data);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    };
    Cubemap::Cubemap(const std::filesystem::path& path) : Texture(ResourceType::Cubemap)
    {
        ZoneScoped;

        LoadFromFile(path);
    }

    Cubemap::Cubemap(ImportData& importData)
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
        glDeleteTextures(1, &m_textureID);
    }

    void Cubemap::Bind(uint32_t slot)
    {
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
    }

    void Cubemap::LoadFromFile(const std::filesystem::path& path)
    {
        m_FilePath = path;
        m_Name = path.filename().string();

        m_Properties.srgb = false;

        if(path.extension() == ".hdr")
        {
            LoadHDRFromFile(path);
        }
        else
        {
            LoadStandardFromFile(path);
        }
    }

    void Cubemap::LoadHDRFromFile(const std::filesystem::path& path)
    {
        int nrChannels;
        stbi_set_flip_vertically_on_load(true);
        float* data = stbi_loadf(path.string().c_str(), &m_Width, &m_Height, &nrChannels, 0);
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

        EquirectToCubemap(data, m_Width, m_Height);

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
        
        // Create a framebuffer to render the cubemap
        
        // Resolution of the cubemap faces
        int cubemapFaceSize = 512; // Adjust as needed

        uint32_t fbo, rbo;
        glCreateFramebuffers(1, &fbo);
        glCreateRenderbuffers(1, &rbo);
        glNamedRenderbufferStorage(rbo, GL_DEPTH_COMPONENT24, cubemapFaceSize, cubemapFaceSize);
        glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);

        // Create the cubemap texture
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_textureID);
        glTextureStorage2D(m_textureID, 1, GL_RGB32F, cubemapFaceSize, cubemapFaceSize);

        for(int i = 0; i < 6; ++i)
        {
            // glTextureSubImage3D(name, 0, 0, 0, face, bitmap.width, bitmap.height, 1, bitmap.format, GL_UNSIGNED_BYTE, bitmap.pixels);
            //glTextureSubImage3D(m_textureID, 0, 0, 0, i, cubemapFaceSize, cubemapFaceSize, 1, GL_RGB, GL_FLOAT, nullptr);
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB32F, cubemapFaceSize, cubemapFaceSize, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTextureParameteri(m_textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(m_textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = 
        {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        static Ref<Shader> shader = CreateRef<Shader>("EquirectangularToCubemap", equirectToCubemapSource);
        static Ref<Mesh> cube = PrimitiveMesh::CreateCube({-1.0f, -1.0f, -1.0f});

        shader->Bind();
        shader->setMat4("projection", captureProjection);
        shader->setInt("equirectangularMap", 0);
        glBindTextureUnit(0, equirectTextureID);

        glViewport(0, 0, cubemapFaceSize, cubemapFaceSize);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        for (uint32_t i = 0; i < 6; ++i)
        {
            shader->setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, m_textureID, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            cube->GetVertexArray()->Bind();
            glDrawElements(GL_TRIANGLES, cube->GetVertexArray()->GetIndexBuffer()->GetCount(), GL_UNSIGNED_INT, 0);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
