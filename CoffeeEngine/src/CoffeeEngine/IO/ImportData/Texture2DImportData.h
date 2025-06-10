#pragma once

#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>

namespace Coffee
{

    struct Texture2DImportData : public ImportData
    {
        bool sRGB = true;

        Texture2DImportData() : ImportData(ResourceType::Texture2D) {}

        template <typename Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<ImportData>(this), CEREAL_NVP(sRGB));
        }
    };

} // namespace Coffee
CEREAL_REGISTER_TYPE(Coffee::Texture2DImportData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::ImportData, Coffee::Texture2DImportData);