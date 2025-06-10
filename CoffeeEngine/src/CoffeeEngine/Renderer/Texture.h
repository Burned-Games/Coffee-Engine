#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/Serialization/FilesystemPathSerialization.h"

#include <cereal/access.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

// Remove
#include <glad/glad.h>

namespace Coffee {

    enum class ImageFormat
    {
        R8,
        RG8,
        RGB8,
        SRGB8,
        RGBA8,
        SRGBA8,
        R16F,
        RG16F,
        RGB16F,
        RGBA16F,
        R32F,
        RGB32F,
        RGBA32F,
        DEPTH24STENCIL8
    };

    enum class TextureWrap
    {
        Repeat,
        MirroredRepeat,
        ClampToEdge,
        ClampToBorder
    };

    enum class TextureFilter
    {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapNearest,
        NearestMipmapLinear,
        LinearMipmapLinear
    };

    struct TextureProperties
    {
        ImageFormat Format;
        TextureWrap Wrapping = TextureWrap::Repeat;
        TextureFilter MinFilter = TextureFilter::LinearMipmapLinear;
        TextureFilter MagFilter = TextureFilter::Linear;
        TextureFilter MipMapFilter = TextureFilter::Linear;
        glm::vec4 BorderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        uint32_t Width, Height;
        bool GenerateMipmaps = true;
        bool srgb = true;

        template<class Archive>
        void serialize(Archive& archive)
        {
            int formatInt = static_cast<int>(Format);
            archive(formatInt, Width, Height, GenerateMipmaps, srgb);
            Format = static_cast<ImageFormat>(formatInt);
        } 
    };
    


    class Texture : public Resource
    {
    public:
        Texture() : Resource(ResourceType::Texture) {};
        Texture(ResourceType type) : Resource(type) {};
        virtual ~Texture() = default;

        virtual void Bind(uint32_t slot) = 0;
        virtual uint32_t GetWidth() = 0;
        virtual uint32_t GetHeight() = 0;
        virtual uint32_t GetID() = 0;
        virtual ImageFormat GetImageFormat() = 0;
    private:
        friend class cereal::access;
        
        template<class Archive>
        void save(Archive& archive) const
        {
            archive(cereal::base_class<Resource>(this));
        }

        template<class Archive>
        void load(Archive& archive)
        {
            archive(cereal::base_class<Resource>(this));
        }
    };

    class Texture2D : public Texture
    {
    public:
        Texture2D() = default;
        Texture2D(const TextureProperties& properties);
        Texture2D(uint32_t width, uint32_t height, ImageFormat imageFormat, TextureWrap wrapping = TextureWrap::Repeat,
                  TextureFilter minFilter = TextureFilter::LinearMipmapLinear, TextureFilter magFilter = TextureFilter::Linear,
                  TextureFilter mipMapFilter = TextureFilter::Linear, const glm::vec4& borderColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        Texture2D(const std::filesystem::path& path, bool srgb = true);
        Texture2D(ImportData& importData);
        ~Texture2D();

        void Bind(uint32_t slot) override;
        void Resize(uint32_t width, uint32_t height);
        std::pair<uint32_t, uint32_t> GetSize() { return std::make_pair(m_Width, m_Height); };
        uint32_t GetWidth() override { return m_Width; };
        uint32_t GetHeight() override { return m_Height; };
        uint32_t GetID() override { return m_textureID; };
        ImageFormat GetImageFormat() override { return m_Properties.Format; };

        void Clear(glm::vec4 color);
        void SetData(void* data, uint32_t size);

        static Ref<Texture2D> Load(const std::filesystem::path& path);
        static Ref<Texture2D> Create(uint32_t width, uint32_t height, ImageFormat format);
        static Ref<Texture2D> Create(const TextureProperties& properties);

    private:
        void LoadFromFile(const std::filesystem::path& path);
        void InitializeTexture2D();

        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(m_Properties, m_Data, m_Width, m_Height, cereal::base_class<Texture>(this));
        }

        template <class Archive>
        void load(Archive& archive)
        {
            archive(m_Properties, m_Data, m_Width, m_Height, cereal::base_class<Texture>(this));
        }

        template <class Archive>
        static void load_and_construct(Archive& data, cereal::construct<Texture2D>& construct)
        {
            TextureProperties properties;
            data(properties);
            construct(properties.Width, properties.Height, properties.Format);

            data(construct->m_Data, construct->m_Width, construct->m_Height,
                 cereal::base_class<Texture>(construct.ptr()));
            construct->m_Properties = properties;
            construct->SetData(construct->m_Data.data(), construct->m_Data.size());
        }
    private:
        TextureProperties m_Properties;
        std::vector<unsigned char> m_Data;
        uint32_t m_textureID;
        int m_Width, m_Height;
    };

    class Mesh;

    class Cubemap : public Texture
    {
    public:
        Cubemap();
        Cubemap(const std::filesystem::path& path);
        Cubemap(ImportData& importData);
        ~Cubemap();

        void Bind(uint32_t slot) override;
        // Temporal, in the future i think it would be nice that the irradiance is a Cubemap object
        void BindIrradianceMap(uint32_t slot);
        void BindPrefilteredMap(uint32_t slot);
        uint32_t GetID() override { return m_CubeMapID; };
        uint32_t GetIrradianceMapID() { return m_IrradianceMapID; };
        uint32_t GetPrefilteredMapID() { return m_PrefilteredMapID; };

        uint32_t GetWidth() override { return m_Properties.Width; };
        uint32_t GetHeight() override { return m_Properties.Height; };
        ImageFormat GetImageFormat() override { return m_Properties.Format; };

        static Ref<Cubemap> Load(const std::filesystem::path& path);
        static Ref<Cubemap> Create(const std::filesystem::path& path);
    private:
        void LoadFromFile(const std::filesystem::path& path);
        void LoadFromData(const std::vector<float>& data);
        
        void LoadEquirectangularMapFromFile(const std::filesystem::path& path);
        void EquirectToCubemap(float* data, int width, int height);

        void GenerateIrradianceMap();
        void GeneratePrefilteredMap();

        friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(m_Properties);

            // Get all the data from opengl and save it

            // Cubemap data (faces)
            std::vector<float> cubeMapData = std::vector<float>(m_Properties.Width * m_Properties.Height * 3 * 6);
            glGetTextureImage(m_CubeMapID, 0, GL_RGB, GL_FLOAT, cubeMapData.size() * sizeof(float), cubeMapData.data());

            archive(cubeMapData);

            // Irradiance map data (faces only)

            std::vector<float> irradianceMapData = std::vector<float>(m_IrradianceMapResolution * m_IrradianceMapResolution * 3 * 6);
            glGetTextureImage(m_IrradianceMapID, 0, GL_RGB, GL_FLOAT, irradianceMapData.size() * sizeof(float), irradianceMapData.data());

            archive(irradianceMapData, m_IrradianceMapResolution);

            // Prefiltered map data (faces and mip levels)
            std::vector<float> prefilteredMapData;
            unsigned int maxMipLevels = 5; // TODO: make it a member variable
            for (int j = 0; j < maxMipLevels; ++j)
            {
                int width = m_PrefilteredMapResolution >> j;
                int height = m_PrefilteredMapResolution >> j;

                std::vector<float> data(width * height * 3 * 6);
                glGetTextureImage(m_PrefilteredMapID, j, GL_RGB, GL_FLOAT, data.size() * sizeof(float), data.data());
                prefilteredMapData.insert(prefilteredMapData.end(), data.begin(), data.end());
            }

            archive(prefilteredMapData, m_PrefilteredMapResolution);

            archive(cereal::base_class<Texture>(this));
        }

        template <class Archive>
        void load(Archive& archive)
        {
           archive(m_Properties);

            // Load the cubemap data
            std::vector<float> cubeMapData;
            archive(cubeMapData);

            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_CubeMapID);
            glTextureStorage2D(m_CubeMapID, 1, GL_RGB32F, m_Properties.Width, m_Properties.Height);
            glTextureParameteri(m_CubeMapID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(m_CubeMapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_CubeMapID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            int offset = 0;
            int faceSize = m_Properties.Width * m_Properties.Height * 3;

            for (int i = 0; i < 6; ++i)
            {
                glTextureSubImage3D(m_CubeMapID, 0, 0, 0, i, m_Properties.Width, m_Properties.Height, 1, GL_RGB, GL_FLOAT, cubeMapData.data() + offset);
                offset += faceSize;
            }

            // Load the irradiance map data
            std::vector<float> irradianceMapData;
            archive(irradianceMapData, m_IrradianceMapResolution);

            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_IrradianceMapID);
            glTextureStorage2D(m_IrradianceMapID, 1, GL_RGB32F, m_IrradianceMapResolution, m_IrradianceMapResolution);
            glTextureParameteri(m_IrradianceMapID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(m_IrradianceMapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_IrradianceMapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_IrradianceMapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            offset = 0;
            faceSize = m_IrradianceMapResolution * m_IrradianceMapResolution * 3;

            for (int i = 0; i < 6; ++i)
            {
                glTextureSubImage3D(m_IrradianceMapID, 0, 0, 0, i, m_IrradianceMapResolution, m_IrradianceMapResolution, 1, GL_RGB, GL_FLOAT, irradianceMapData.data() + offset);
                offset += faceSize;
            }

            // Load the prefiltered map data
            std::vector<float> prefilteredMapData;
            archive(prefilteredMapData, m_PrefilteredMapResolution);

            unsigned int maxMipLevels = 5; // TODO: make it a member variable

            glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_PrefilteredMapID);
            glTextureStorage2D(m_PrefilteredMapID, maxMipLevels, GL_RGB32F, m_PrefilteredMapResolution, m_PrefilteredMapResolution);
            glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTextureParameteri(m_PrefilteredMapID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

            offset = 0;
            
            for (int mip = 0; mip < maxMipLevels; ++mip)
            {
                int width = m_PrefilteredMapResolution >> mip;
                int height = m_PrefilteredMapResolution >> mip;
                int faceSize = width * height * 3;

               for (int faceIdx = 0; faceIdx < 6; ++faceIdx)
                {
                    glTextureSubImage3D(m_PrefilteredMapID, mip, 0, 0, faceIdx, width, height, 1, GL_RGB, GL_FLOAT, prefilteredMapData.data() + offset);
                    offset += faceSize;
                }
            }

            archive(cereal::base_class<Texture>(this));
        }

/*         template <class Archive>
        static void load_and_construct(Archive& data, cereal::construct<Cubemap>& construct)
        {
        } */

    private:
        TextureProperties m_Properties;
        
        uint32_t m_CubeMapID;
        uint32_t m_IrradianceMapID;
        uint32_t m_PrefilteredMapID;

        uint32_t m_IrradianceMapResolution = 32;
        uint32_t m_PrefilteredMapResolution = 128;
        
        // common variables for map generation
        uint32_t fbo, rbo;
        static Ref<Mesh> m_CubeMesh;
    };

}

CEREAL_REGISTER_TYPE(Coffee::Texture);
CEREAL_REGISTER_TYPE(Coffee::Texture2D);
CEREAL_REGISTER_TYPE(Coffee::Cubemap);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Texture);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Texture, Coffee::Texture2D);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Texture, Coffee::Cubemap);