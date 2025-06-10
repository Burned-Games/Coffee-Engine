#pragma once

#include "CoffeeEngine/Core/UUID.h"

#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>
#include <filesystem>
#include <CoffeeEngine/IO/Resource.h>

namespace Coffee {

    struct ImportData
    {
        UUID uuid = UUID::null;
        ResourceType type = ResourceType::Unknown;

        std::filesystem::path originalPath;
        std::filesystem::path cachedPath;

        bool cache = true;
        bool internal = false;

        ImportData() = default;
        ImportData(ResourceType type) : type(type) {}
        virtual ~ImportData() = default;
        
        template<typename Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(CEREAL_NVP(uuid), CEREAL_NVP(type), CEREAL_NVP(originalPath), CEREAL_NVP(cachedPath), CEREAL_NVP(cache), CEREAL_NVP(internal));
        }

        bool IsValid() const
        {
            // Implement the logic to check if the ImportData has the necessary embedded data
            // This is a placeholder implementation
            return uuid != UUID::null && (!cache || !cachedPath.empty());
        }
    };

}
CEREAL_REGISTER_TYPE(Coffee::ImportData);