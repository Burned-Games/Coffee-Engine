#include "SceneManager.h"
#include <filesystem>
#include <tracy/Tracy.hpp>
#include <future>

namespace Coffee {

    SceneManager::SceneState SceneManager::s_SceneState = SceneManager::SceneState::Edit;
    Ref<Scene> SceneManager::s_ActiveScene = nullptr;
    std::filesystem::path SceneManager::s_WorkingDirectory = std::filesystem::current_path();

    Ref<Scene> SceneManager::PreloadScene(const std::filesystem::path& scenePath)
    {
        ZoneScoped;

        auto scene = Scene::Load(scenePath);

        return scene;
    }

    // This is a mock implementation, it should be implemented properly
    std::future<Ref<Scene>> SceneManager::PreloadSceneAsync(const std::filesystem::path& scenePath)
    {
        // Store the future returned by std::async
        auto future = std::async(std::launch::async, [scenePath]() -> Ref<Scene> {
            ZoneScoped;

            Ref<Scene> scene = Scene::Load(scenePath);
            return scene;
        });

        return future;
    }

    void SceneManager::ChangeScene(const std::filesystem::path& scenePath)
    {
        ZoneScoped;
    
        std::filesystem::path fullPath = s_WorkingDirectory / scenePath;
    
        ExitCurrentScene();
    
        s_ActiveScene = Scene::Load(fullPath);
    
        InitNewScene();
    }
    
    void SceneManager::ChangeScene(const Ref<Scene>& scene)
    {
        ZoneScoped;
    
        ExitCurrentScene();

        if(s_ActiveScene != scene)
        {
            AudioZone::RemoveAllReverbZones();
            Audio::UnregisterAllGameObjects();
        }
        else
        {
            Audio::StopAllEvents();
        }
    
        s_ActiveScene = scene;
    
        InitNewScene();
    }

    // This is a mock implementation, it should be implemented properly
    void SceneManager::ChangeSceneAsync(const std::filesystem::path& scenePath)
    {
        // Store the future returned by std::async
        auto future = std::async(std::launch::async, [scenePath]() -> Ref<Scene> {
            ZoneScoped;
    
            std::filesystem::path fullPath = s_WorkingDirectory / scenePath;
            Ref<Scene> scene = Scene::Load(fullPath);
            return scene;
        });
    
        // Optionally, you can wait for the future to complete and handle any exceptions
        try {
            Ref<Scene> loadedScene = future.get(); // This will rethrow any exceptions that occurred in the async task
            if (loadedScene) {
                ChangeScene(loadedScene);
            } else {
                std::cerr << "Failed to load scene asynchronously: " << scenePath << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to change scene asynchronously: " << e.what() << std::endl;
        }
    }

    void SceneManager::ExitCurrentScene()
    {
        if (s_ActiveScene)
        {
            if (s_SceneState == SceneState::Play)
            {
                s_ActiveScene->OnExitRuntime();
            }
            else if (s_SceneState == SceneState::Edit)
            {
                s_ActiveScene->OnExitEditor();
            }
        }
    }

    void SceneManager::InitNewScene()
    {
        if (s_SceneState == SceneState::Play)
        {
            s_ActiveScene->OnInitRuntime();
        }
        else if (s_SceneState == SceneState::Edit)
        {
            s_ActiveScene->OnInitEditor();
        }
    }
}