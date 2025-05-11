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
        TextureFilter MinFilter = TextureFilter::Linear;
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
                  TextureFilter minFilter = TextureFilter::Linear, TextureFilter magFilter = TextureFilter::Linear,
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

/*         friend class cereal::access;

        template<class Archive>
        void save(Archive& archive) const
        {
            archive(m_Properties, m_Data, m_HDRData, m_Width, m_Height, cereal::base_class<Texture>(this));
        }

        template <class Archive>
        void load(Archive& archive)
        {
            archive(m_Properties, m_Data, m_HDRData, m_Width, m_Height, cereal::base_class<Texture>(this));
        }

        template <class Archive>
        static void load_and_construct(Archive& data, cereal::construct<Cubemap>& construct)
        {
             construct();

            data(construct->m_Properties, construct->m_Data, construct->m_HDRData, construct->m_Width, construct->m_Height,
                 cereal::base_class<Texture>(construct.ptr()));

            const ImageFormat& format = construct->m_Properties.Format;
            if (format == ImageFormat::R8 || format == ImageFormat::RG8 || format == ImageFormat::RGB8 || format == ImageFormat::RGBA8)
            {
                construct->LoadStandardFromData(construct->m_Data);
            }
            else
            {
                construct->LoadHDRFromData(construct->m_HDRData);
            } 
        } */

    private:
        TextureProperties m_Properties;
        //std::vector<std::vector<std::vector<float>>> m_HDRData; // m_HDRData[faceIndex][mipLevel][pixelIndex]
        uint32_t m_CubeMapID;
        uint32_t m_IrradianceMapID;
        uint32_t m_PrefilteredMapID;

        uint32_t IrradianceMapResolution = 32;
        uint32_t PrefilteredMapResolution = 128;
    };

}

CEREAL_REGISTER_TYPE(Coffee::Texture);
CEREAL_REGISTER_TYPE(Coffee::Texture2D);
CEREAL_REGISTER_TYPE(Coffee::Cubemap);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Resource, Coffee::Texture);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Texture, Coffee::Texture2D);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::Texture, Coffee::Cubemap);