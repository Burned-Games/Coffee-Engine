#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Core/Assert.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/IO/ImportData/Texture2DImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"

#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/memory.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
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

    void Cubemap::LoadStandardFromFile(const std::filesystem::path& path)
    {
        // Load the combined image
        int nrChannels;
        unsigned char* data = stbi_load(path.string().c_str(), &m_Width, &m_Height, &nrChannels, 0);
        if (!data) {
            COFFEE_CORE_ERROR("Failed to load cubemap texture: {0} (REASON: {1})", m_FilePath.string(), stbi_failure_reason());
            return;
        }

        m_Data = std::vector<unsigned char>(data, data + m_Width * m_Height * nrChannels);
        stbi_image_free(data);

        switch (nrChannels)
        {
            case 1:
                m_Properties.Format = ImageFormat::R8;
            break;
            case 3:
                m_Properties.Format = ImageFormat::RGB8;
            break;
            case 4:
                m_Properties.Format = ImageFormat::RGBA8;
            break;
        }

        LoadStandardFromData(m_Data);
    }

    void Cubemap::LoadHDRFromFile(const std::filesystem::path& path)
    {
        int nrChannels;
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

    void Cubemap::LoadStandardFromData(const std::vector<unsigned char>& data)
    {
        m_Data = data;

        int nrChannels = ImageFormatToChannelCount(m_Properties.Format);

        int mipLevels = 1 + floor(log2(std::max(m_Width, m_Height)));

        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);

        // Verify layout dimensions (should be square with faces in cross shape)
        int faceSize = m_Width / 4;
        if (m_Width != faceSize * 4 || m_Height != faceSize * 3) {
            COFFEE_CORE_ERROR("Cubemap texture layout is invalid: {0}", m_FilePath.string());
            m_Data.clear();
            return;
        }

        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);

        uint32_t targets[6] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, // +X
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // -X
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // +Y
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // -Y
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // +Z
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  // -Z
        };

        int offsets[6][2] = {
            {2, 1}, // +X
            {0, 1}, // -X
            {1, 0}, // +Y
            {1, 2}, // -Y
            {1, 1}, // +Z
            {3, 1}  // -Z
        };

        for (int i = 0; i < 6; ++i) {
            int offsetX = offsets[i][0] * faceSize;
            int offsetY = offsets[i][1] * faceSize;

            unsigned char* faceBuffer = new unsigned char[faceSize * faceSize * nrChannels];
            for (int y = 0; y < faceSize; ++y) {
                memcpy(
                    faceBuffer + y * faceSize * nrChannels,
                    m_Data.data() + ((offsetY + y) * m_Width + offsetX) * nrChannels,
                    faceSize * nrChannels
                );
            }

            glTexImage2D(
                targets[i],
                0, internalFormat, faceSize, faceSize,
                0, format, GL_UNSIGNED_BYTE, faceBuffer
            );

            delete[] faceBuffer;
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void Cubemap::LoadHDRFromData(const std::vector<float>& data)
    {
        m_HDRData = data;

        int nrChannels = ImageFormatToChannelCount(m_Properties.Format);
        
        int mipLevels = 1 + floor(log2(std::max(m_Width, m_Height)));
        
        GLenum internalFormat = ImageFormatToOpenGLInternalFormat(m_Properties.Format);
        GLenum format = ImageFormatToOpenGLFormat(m_Properties.Format);
        
        int faceSize = m_Width / 4;
        if (m_Width != faceSize * 4 || m_Height != faceSize * 3) {
            COFFEE_CORE_ERROR("Cubemap texture layout is invalid: {0}", m_FilePath.string());
            m_HDRData.clear();
            return;
        }
        
        glGenTextures(1, &m_textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, m_textureID);
        
        uint32_t targets[6] = {
            GL_TEXTURE_CUBE_MAP_POSITIVE_X, // +X
            GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // -X
            GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // +Y
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, // -Y
            GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // +Z
            GL_TEXTURE_CUBE_MAP_NEGATIVE_Z  // -Z
        };
        
        int offsets[6][2] = {
            {2, 1}, // +X
            {0, 1}, // -X
            {1, 0}, // +Y
            {1, 2}, // -Y
            {1, 1}, // +Z
            {3, 1}  // -Z
        };
        
        for (int i = 0; i < 6; ++i) {
            int offsetX = offsets[i][0] * faceSize;
            int offsetY = offsets[i][1] * faceSize;
        
            float* faceBuffer = new float[faceSize * faceSize * nrChannels];
            for (int y = 0; y < faceSize; ++y) {
                memcpy(
                    faceBuffer + y * faceSize * nrChannels,
                    m_HDRData.data() + ((offsetY + y) * m_Width + offsetX) * nrChannels,
                    faceSize * nrChannels * sizeof(float)
                );
            }
        
            glTexImage2D(
                targets[i],
                0, internalFormat, faceSize, faceSize,
                0, format, GL_FLOAT, faceBuffer
            );
        
            delete[] faceBuffer;
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void Cubemap::EquirectToCubemap(float* data, int width, int height)
    {
        
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
