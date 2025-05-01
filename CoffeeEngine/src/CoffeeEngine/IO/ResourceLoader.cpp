#include "ResourceLoader.h"
#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/IO/CacheManager.h"
#include "CoffeeEngine/IO/ImportData/ImportData.h"
#include "CoffeeEngine/IO/ImportData/ImportDataUtils.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/IO/ResourceImporter.h"
#include "CoffeeEngine/IO/ResourceUtils.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace Coffee {

    std::filesystem::path ResourceLoader::s_EngineAssetsDirectory = std::filesystem::current_path() / "assets";
/* 
    bool ResourceLoader::isInternalResource(const std::filesystem::path& path) {
            return std::filesystem::equivalent(path.parent_path(), s_EngineAssetsDirectory);
        } */
    
    bool ResourceLoader::isInternalResource(const std::filesystem::path& path)
    {
        if (path.is_absolute()) {
            return std::filesystem::equivalent(path.parent_path(), s_EngineAssetsDirectory);
        } else {
            return path.string().find("assets/") == 0; // Think if this is the best way...
        }
    }

    std::filesystem::path ResourceLoader::s_WorkingDirectory = std::filesystem::current_path();
    ResourceImporter ResourceLoader::s_Importer = ResourceImporter();

    void ResourceLoader::LoadFile(const std::filesystem::path& path)
    {
        if (!is_regular_file(path))
        {
            COFFEE_CORE_ERROR("ResourceLoader::LoadResources: {0} is not a file!", path.string());
            return;
        }

        const ResourceType type = GetResourceTypeFromExtension(path);

        if(type == ResourceType::Unknown and path.extension() != ".import")
        {
            COFFEE_CORE_ERROR("ResourceLoader::LoadResources: Unsupported file extension {0}", path.extension().string());
            return;
        }

        if(path.extension() == ".import")
        {
            COFFEE_CORE_INFO("ResourceLoader::LoadDirectory: Loading resource from import file {0}", path.string());

            Scope<ImportData> importData = ImportDataUtils::LoadImportData(path);
            switch (importData->type)
            {
                case ResourceType::Texture2D:
                {
                    Load<Texture2D>(*importData);
                    break;
                }
                case ResourceType::Cubemap:
                {
                    Load<Cubemap>(*importData);
                    break;
                }
                case ResourceType::Model:
                {
                    Load<Model>(*importData);
                    break;
                }
                case ResourceType::Shader:
                {
                    Load<Shader>(*importData);
                    break;
                }
                case ResourceType::PBRMaterial:
                {
                    Load<PBRMaterial>(*importData);
                    break;
                }
                case ResourceType::ShaderMaterial:
                {
                    Load<ShaderMaterial>(*importData);
                    break;
                }
                default:
                {
                    COFFEE_CORE_ERROR("ResourceLoader::LoadResources: Unsupported resource type {0}", ResourceTypeToString(importData->type));
                    break;
                }
            }
        }
        else
        {
            std::filesystem::path importFilePath = path;
            importFilePath += ".import";
            if(std::filesystem::exists(importFilePath))
            {
                return;
            }

            switch (type)
            {
                case ResourceType::Texture2D:
                {
                    Load<Texture2D>(path);
                    break;
                }
                case ResourceType::Cubemap:
                {
                    Load<Cubemap>(path);
                    break;
                }
                case ResourceType::Model:
                {
                    Load<Model>(path);
                    break;
                }
                case ResourceType::Shader:
                {
                    Load<Shader>(path);
                    break;
                }
            }
        }
    }

    void ResourceLoader::LoadDirectory(const std::filesystem::path& directory)
    {
        std::vector<std::filesystem::path> files;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            if (!entry.is_regular_file() || GetResourceTypeFromExtension(entry.path()) == ResourceType::Unknown and entry.path().extension() != ".import")
            {
                continue;
            }

            files.push_back(entry.path());
        }

        std::sort(files.begin(), files.end(), [](const std::filesystem::path& a, const std::filesystem::path& b) {
            if (a.extension() == ".import" && b.extension() != ".import") return true;
            if (a.extension() != ".import" && b.extension() == ".import") return false;
            if (GetResourceTypeFromExtension(a) == ResourceType::Model && GetResourceTypeFromExtension(b) != ResourceType::Model) return true;
            if (GetResourceTypeFromExtension(a) != ResourceType::Model && GetResourceTypeFromExtension(b) == ResourceType::Model) return false;
            return a < b;
        });

        for (const auto& path : files)
        {
            LoadFile(path);
        }
    }

    void ResourceLoader::RemoveResource(const Ref<Resource>& resource)
    {   
        std::filesystem::path importFilePath = resource->GetPath();
        importFilePath += ".import";

        Scope<ImportData> importData = ImportDataUtils::LoadImportData(importFilePath);

        // Remove the cache file

        if(std::filesystem::exists(importData->cachedPath))
        {
            std::filesystem::remove(importData->cachedPath);
        }

        // Remove the resource file

        if(std::filesystem::exists(importData->originalPath))
        {
            std::filesystem::remove(importData->originalPath);
        }

        // Remove the resource import file

        if(std::filesystem::exists(importFilePath))
        {
            std::filesystem::remove(importFilePath);
        }

        // TODO: Remove all the dependencies
        
        // Remove the resource from the registry
        
        ResourceRegistry::Remove(resource->GetUUID());
    }

    void ResourceLoader::ReimportResource(const Ref<Resource>& resource)
    {
        std::filesystem::path importFilePath = resource->GetPath();
        importFilePath += ".import";

        Scope<ImportData> importData = ImportDataUtils::LoadImportData(importFilePath);

        // Remove the cache file

        if(std::filesystem::exists(importData->cachedPath))
        {
            std::filesystem::remove(importData->cachedPath);
        }

        // Remove from the registry
        ResourceRegistry::Remove(resource->GetUUID());

        // Reimport the resource

        switch (importData->type)
        {
            case ResourceType::Texture2D:
            {
                Load<Texture2D>(*importData);
                break;
            }
            case ResourceType::Cubemap:
            {
                Load<Cubemap>(*importData);
                break;
            }
            case ResourceType::Model:
            {
                Load<Model>(*importData);
                break;
            }
            case ResourceType::Shader:
            {
                Load<Shader>(*importData);
                break;
            }
            case ResourceType::PBRMaterial:
            {
                Load<PBRMaterial>(*importData);
                break;
            }
            case ResourceType::ShaderMaterial:
            {
                Load<ShaderMaterial>(*importData);
                break;
            }
            default:
            {
                COFFEE_CORE_ERROR("ResourceLoader::ReimportResource: Unsupported resource type {0}", ResourceTypeToString(importData->type));
                break;
            }
        }
    }

}