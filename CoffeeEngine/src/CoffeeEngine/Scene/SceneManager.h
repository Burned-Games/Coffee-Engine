#pragma once

#include "CoffeeEngine/Scene/Scene.h"
#include <filesystem>
#include <future>

namespace Coffee {

    class SceneManager
    {
    public:
        enum class SceneState
        {
            Edit = 0,
            Play = 1
        };
    public:
        
        static Ref<Scene> PreloadScene(const std::filesystem::path& scenePath);
        static std::future<Ref<Scene>> PreloadSceneAsync(const std::filesystem::path& scenePath);

        static void ChangeScene(const std::filesystem::path& scenePath);
        static void ChangeScene(const Ref<Scene>& scene);
        static void ChangeSceneAsync(const std::filesystem::path& scenePath);

        static Ref<Scene>& GetActiveScene() { return s_ActiveScene; }

        static void SetWorkingDirectory(const std::filesystem::path& workingDirectory) { s_WorkingDirectory = workingDirectory; }

        static void SetSceneState(SceneState state) { s_SceneState = state; }
        static SceneState GetSceneState() { return s_SceneState; }
        static std::string GetSceneName() { return s_ActiveScene ? s_ActiveScene->GetFilePath().filename().string() : ""; }
    private:
        static void ExitCurrentScene();
        static void InitNewScene();
    private:
        static SceneState s_SceneState;
        static std::filesystem::path s_WorkingDirectory;
        static Ref<Scene> s_ActiveScene;
    };

}