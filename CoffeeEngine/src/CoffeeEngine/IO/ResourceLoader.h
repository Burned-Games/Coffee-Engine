/**
 * @defgroup io IO
 * @brief IO components of the CoffeeEngine.
 * @{
 */

#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/UUID.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceImporter.h"
#include "ImportData/ImportData.h"
#include "ImportData/ImportDataUtils.h"
#include "ResourceRegistry.h"

#include <filesystem>



namespace Coffee {

    class Model;
    class Mesh;
    class Material;
    class Texture;
    class Texture2D;
    class Animation;

    /**
     * @class ResourceLoader
     * @brief Loads resources such as textures and models for the CoffeeEngine.
     */
    class ResourceLoader
    {
    public:
        /**
         * @brief Loads all resources from a directory.
         * @param directory The directory to load resources from.
         */
        static void LoadDirectory(const std::filesystem::path& directory);

        /**
         * @brief Loads a single resource file.
         * @param path The file path of the resource to load.
         */
        static void LoadFile(const std::filesystem::path& path);

        template <typename T>
        inline static Ref<T> Load(const std::filesystem::path& path)
        {
            if (ImportDataUtils::HasImportFile(path))
            {
                std::filesystem::path importPath = path;
                importPath += ".import";
                Scope<ImportData> importData = ImportDataUtils::LoadImportData(importPath);
                return Load<T>(*importData);
            }
            else
            {
                Scope<ImportData> newImportData = ImportDataUtils::CreateImportData<T>();
                newImportData->originalPath = path;

                if(isInternalResource(path))
                {
                    newImportData->internal = true;
                }

                const Ref<T>& resource = s_Importer.Import<T>(*newImportData);

                ImportDataUtils::SaveImportData(newImportData);
                ResourceRegistry::Add(newImportData->uuid, resource);

                return resource;
            }
        }

        template<typename T>
        inline static Ref<T> Load(const ImportData& importData)
        {
            if (ResourceRegistry::Exists(importData.uuid))
            {
                return ResourceRegistry::Get<T>(importData.uuid);
            }

            const Ref<T>& resource = s_Importer.Import<T>(importData);
            
            ResourceRegistry::Add(importData.uuid, resource);
            return resource;
        }

        template<typename T>
        static Ref<T> LoadEmbedded(const ImportData& importData)
        {
            if (ResourceRegistry::Exists(importData.uuid))
            {
                return ResourceRegistry::Get<T>(importData.uuid);
            }

            const Ref<T>& resource = s_Importer.ImportEmbedded<T>(importData);

            ResourceRegistry::Add(importData.uuid, resource);
            return resource;
        }

        template<typename T>
        static Ref<T> GetResource(UUID uuid)
        {
            if (uuid == UUID::null)
                return nullptr;

            if (ResourceRegistry::Exists(uuid))
            {
                return ResourceRegistry::Get<T>(uuid);
            }

            const Ref<T>& resource = s_Importer.ImportFromCache<T>(uuid);

            if (resource)
            {
                ResourceRegistry::Add(uuid, resource);
                return resource;
            }

            return nullptr;
        }

        static void RemoveResource(const Ref<Resource>& resource);
        static void ReimportResource(const Ref<Resource>& resource);

        static void SetWorkingDirectory(const std::filesystem::path& path) { s_WorkingDirectory = path; }
        static const std::filesystem::path& GetWorkingDirectory() { return s_WorkingDirectory; }
    
    private:
        static bool isInternalResource(const std::filesystem::path& path);
    private:
        static std::filesystem::path s_EngineAssetsDirectory; ///< The directory where the engine Resources are stored.
        static std::filesystem::path s_WorkingDirectory; ///< The working directory of the resource loader.
        static ResourceImporter s_Importer; ///< The importer used to load resources.
    };

}

/** @} */