#pragma once

#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include <cereal/cereal.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/polymorphic.hpp>
#include <unordered_map>

namespace Coffee {

    struct ModelImportData: public ImportData
    {
        std::unordered_map<std::string, UUID> meshUUIDs;
        std::unordered_map<std::string, UUID> materialUUIDs;

        ModelImportData() : ImportData(ResourceType::Model) {}

        template<typename Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(CEREAL_NVP(meshUUIDs), CEREAL_NVP(materialUUIDs), cereal::base_class<ImportData>(this));
        }
    };

}

CEREAL_REGISTER_TYPE(Coffee::ModelImportData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::ImportData, Coffee::ModelImportData);