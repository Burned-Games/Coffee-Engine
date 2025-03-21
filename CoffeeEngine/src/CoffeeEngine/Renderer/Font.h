#pragma once

#include "CoffeeEngine/Renderer/Texture.h"
#include <filesystem>

namespace Coffee {

    struct MSDFData;

    class Font
    {
    public:
        Font(const std::filesystem::path& path);
        ~Font();

        const MSDFData* GetMSDFData() const { return m_Data; }
        Ref<Texture2D> GetAtlasTexture() const { return m_AtlasTexture; }

        float GetTextWidth(const std::string& text, float fontSize) const;

        static Ref<Font> GetDefault();
    private:
        MSDFData* m_Data;
        Ref<Texture2D> m_AtlasTexture;
    };

}