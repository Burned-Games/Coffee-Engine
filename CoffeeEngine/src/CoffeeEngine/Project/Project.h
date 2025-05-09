#pragma once

#include "CoffeeEngine/Core/Base.h"
#include <cereal/cereal.hpp>
#include <filesystem>
#include <string>

namespace Coffee {

    /**
     * @defgroup project Project
     * @brief Project management components of the CoffeeEngine.
     * @{
     */

    /**
     * @brief The Project class is responsible for managing project data such as name, directory, and start scene path.
     */
    class Project {
    public:
        /**
         * @brief Creates a new project.
         * @return A reference to the newly created project.
         */
        static Ref<Project> New(const std::filesystem::path& path);

        /**
         * @brief Loads a project from the specified path.
         * @param path The path to the project file.
         * @return A reference to the loaded project.
         */
        static Ref<Project> Load(const std::filesystem::path& path);

        /**
         * @brief Saves the active project to the specified path.
         * @param path The path to save the project file.
         */
        static void SaveActive();

        /**
         * @brief Gets the active project.
         * @return A reference to the active project.
         */
        static Ref<Project> GetActive() { return s_ActiveProject; }

        /**
         * @brief Gets the directory of the active project.
         * @return The path to the project directory.
         */
        static const std::filesystem::path& GetProjectDirectory() { return s_ActiveProject->m_ProjectDirectory; }

        /**
         * @brief Gets the name of the active project.
         * @return The name of the project.
         */
        static const std::string& GetProjectName() { return s_ActiveProject->m_Name; }

        /**
         * @brief Sets the name of the active project.
         * @param name The new name for the project
         */
        static void SetProjectName(const std::string& name) { s_ActiveProject->m_Name = name; }

        static const std::filesystem::path& GetProjectDefaultScene() { return s_ActiveProject->m_StartScenePath; }

        /**
         * @brief Retrieves the cache directory path of the active project.
         * 
         * This static method returns a constant reference to the cache directory path
         * associated with the currently active project.
         * 
         * @return const std::filesystem::path& Reference to the cache directory path.
         */
        static std::filesystem::path GetCacheDirectory() { return s_ActiveProject->GetProjectDirectory() / s_ActiveProject->m_CacheDirectory; }

        /**
         * @brief Retrieves de audio directory path of the active project
         *
         * This static method returns a reference to the audio directory absolute path associated with the currently
         * active project
         * If no audio directory has been defined, it returns the project's directory path instead
         *
         * @return audio directory absolute path
         */
        static std::filesystem::path GetAudioDirectory() { return GetProjectDirectory() / GetRelativeAudioDirectory(); }

        /**
         * @brief Retrieves the audio directory relative path of the active object
         *
         * This static method returns a reference to the audio directory relative path associated with the currently
         * active project. If no audio directory has been defined, it returns an empty path instead
         *
         * @return The audio directory relative path
         */
        static std::filesystem::path GetRelativeAudioDirectory() { return s_ActiveProject->m_AudioFolderPath; }

        /**
         * @brief Sets the project's audio directory to the path specified
         *
         * @param path The relative path to the audio directory
         */
        static void SetRelativeAudioDirectory(const std::filesystem::path& path) { GetActive()->m_AudioFolderPath = path; }

        /**
         * @brief Serializes the project data.
         * @tparam Archive The type of the archive.
         * @param archive The archive to serialize to.
         */
        template<class Archive> void serialize(Archive& archive, std::uint32_t const version) 
        {
            archive(cereal::make_nvp("Name", m_Name),
                    cereal::make_nvp("StartScene",m_StartScenePath.string()),
                    cereal::make_nvp("CacheDirectory", m_CacheDirectory));

            if (version >= 1)
            {
                archive(cereal::make_nvp("AudioDirectory", m_AudioFolderPath));
            }
            else
            {
                m_AudioFolderPath = "";
            }
        }

    private:
        std::string m_Name = "Untitled"; ///< The name of the project.
        std::filesystem::path m_ProjectDirectory; ///< The directory of the project.
        std::filesystem::path m_CacheDirectory; ///< The directory of the project cache.

        std::filesystem::path m_StartScenePath; ///< The path to the start scene.
        std::filesystem::path m_AudioFolderPath; ///< The path to the audio folder

        inline static Ref<Project> s_ActiveProject; ///< The active project.
    };

    /** @} */
}
CEREAL_CLASS_VERSION(Coffee::Project, 1)