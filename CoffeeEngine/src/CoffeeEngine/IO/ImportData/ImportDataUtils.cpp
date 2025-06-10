#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/IO/ImportData/ImportDataUtils.h"
#include "CoffeeEngine/IO/ImportData/CubemapImportData.h"
#include "CoffeeEngine/IO/ImportData/MaterialImportData.h"
#include "CoffeeEngine/IO/ImportData/ModelImportData.h"
#include "CoffeeEngine/IO/ImportData/ShaderImportData.h"
#include "CoffeeEngine/IO/ImportData/Texture2DImportData.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "ImportData.h"

#include <cereal/cereal.hpp>

#include <filesystem>
#include <fstream>

namespace Coffee {

    void ImportDataUtils::SaveImportData(Scope<ImportData>& importData)
    {
        std::filesystem::path importFilePath = importData->originalPath;
        importFilePath += ".import";

        // backup the original path
        std::filesystem::path originalPathCopy = importData->originalPath;
        std::filesystem::path cachedPathCopy = importData->cachedPath;

        std::filesystem::path fileDirectory = importData->internal ? std::filesystem::current_path() : ResourceLoader::GetWorkingDirectory();

        importData->originalPath = std::filesystem::relative(importData->originalPath, fileDirectory);
        importData->cachedPath = std::filesystem::relative(importData->cachedPath, fileDirectory);

        std::ofstream importFile(importFilePath);
        cereal::JSONOutputArchive archive(importFile);
        archive(CEREAL_NVP(importData));

        // restore the original path
        importData->originalPath = originalPathCopy;
        importData->cachedPath = cachedPathCopy;
    }

    // TODO: Think if the path should be the .import path or the resource and replace the extension inside the function
    Scope<ImportData> ImportDataUtils::LoadImportData(const std::filesystem::path& path)
    {
        //ResourceType type = GetResourceTypeFromExtension(path);
        Scope<ImportData> importData;

        std::filesystem::path importFilePath = path;

        if(path.extension() != ".import") {
            COFFEE_CORE_ERROR("ResourceLoader::LoadImportData: Import file {0} does not have the .import extension!", path.string());
            throw std::runtime_error("Import file does not have the .import extension");
        }
    
        if (!std::filesystem::exists(importFilePath))
        {
            COFFEE_CORE_ERROR("ResourceLoader::LoadImportData: Import file {0} does not exist!", importFilePath.string());
            throw std::runtime_error("Import file does not exist");
        }
    
        std::ifstream importFile(importFilePath);
        if (!importFile.is_open())
        {
            COFFEE_CORE_ERROR("ResourceLoader::LoadImportData: Failed to open import file {0}", importFilePath.string());
            throw std::runtime_error("Failed to open import file");
        }
    
        try
        {
            cereal::JSONInputArchive archive(importFile);
            archive(CEREAL_NVP(importData));
        }
        catch (const cereal::Exception& e)
        {
            COFFEE_CORE_ERROR("ResourceLoader::LoadImportData: Failed to parse import file {0}: {1}", importFilePath.string(), e.what());
            throw;
        }

        std::filesystem::path fileDirectory = importData->internal ? std::filesystem::current_path() : ResourceLoader::GetWorkingDirectory();
    
        // Convert the relative path to an absolute path
        importData->originalPath = fileDirectory / importData->originalPath;
        if (importData->cache)
            importData->cachedPath = fileDirectory / importData->cachedPath;
        return importData;
    }

    bool ImportDataUtils::HasImportFile(const std::filesystem::path& path)
    {
        std::filesystem::path importFilePath = path;
        importFilePath += ".import";

        return std::filesystem::exists(importFilePath);
    }

    Scope<ImportData> ImportDataUtils::CreateImportData(ResourceType type)
    {
        switch(type)
        {
            case ResourceType::Texture2D:
            {
                return CreateScope<Texture2DImportData>();
            }
            case ResourceType::Cubemap:
            {
                return CreateScope<CubemapImportData>();
            }
            case ResourceType::Model:
            {
                return CreateScope<ModelImportData>();
            }
            case ResourceType::PBRMaterial:
            {
                return CreateScope<PBRMaterialImportData>();
            }
            case ResourceType::Shader:
            {
                return CreateScope<ShaderImportData>();
            }
            default:
            {
                COFFEE_CORE_ERROR("ResourceLoader::CreateImportData: Unsupported resource type.");
                return CreateScope<ImportData>();
            }
        }
    }

}