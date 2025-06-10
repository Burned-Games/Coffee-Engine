#pragma once

#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include <cereal/cereal.hpp>
#include <cereal/types/polymorphic.hpp>

namespace Coffee
{

    struct MeshImportData : public ImportData
    {

        std::string name;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Ref<Material> material;
        AABB aabb;

        MeshImportData() : ImportData(ResourceType::Mesh) {}

        template <typename Archive> void serialize(Archive& archive, std::uint32_t const version)
        {
            archive(cereal::base_class<ImportData>(this));
        }
    };

} // namespace Coffee
CEREAL_REGISTER_TYPE(Coffee::MeshImportData);
CEREAL_REGISTER_POLYMORPHIC_RELATION(Coffee::ImportData, Coffee::MeshImportData);