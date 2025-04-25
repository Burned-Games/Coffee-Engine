#pragma once

#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>

namespace Coffee
{

    struct CubemapImportData : public ImportData
    {
        CubemapImportData() : ImportData(ResourceType::Cubemap)
        {
            cache = false;
        }

        template <typename Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<ImportData>(this));
        }
    };

} // namespace Coffee
CEREAL_REGISTER_TYPE(Coffee::CubemapImportData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::ImportData, Coffee::CubemapImportData);