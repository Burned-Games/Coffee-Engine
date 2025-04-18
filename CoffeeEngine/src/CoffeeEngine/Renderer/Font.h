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

        static Ref<Font> GetDefault();
        double CalculateTextWidth(const std::string& text, double fsScale, float kerning);

    private:
        MSDFData* m_Data;
        Ref<Texture2D> m_AtlasTexture;
    };

}