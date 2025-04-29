#include "LuaScene.h"

#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scene/Entity.h"

void Coffee::RegisterSceneBindings(sol::state& luaState)
{
    luaState.new_usertype<Scene>("Scene",
            "create_entity", &Scene::CreateEntity,
            "destroy_entity", &Scene::DestroyEntity,
            "duplicate_entity", &Scene::Duplicate,
            "get_entity_by_name", &Scene::GetEntityByName,
            "get_all_entities", &Scene::GetAllEntities
        );

    luaState.new_usertype<SceneManager>("SceneManager",
        "preload_scene", [](const std::string& scenePath) {
            return SceneManager::PreloadScene(scenePath);
        },
        "preload_scene_async", [](const std::string& scenePath) {
            return SceneManager::PreloadSceneAsync(scenePath);
        },
        "change_scene", sol::overload(
            [](const std::string& scenePath) {
                AudioZone::RemoveAllReverbZones();
                Audio::UnregisterAllGameObjects();
                SceneManager::ChangeScene(scenePath);
            },
            [](const Ref<Scene>& scene) {
                AudioZone::RemoveAllReverbZones();
                Audio::UnregisterAllGameObjects();
                SceneManager::ChangeScene(scene);
            }
        ),
        "change_scene_async", [](const std::string& scenePath) {
            SceneManager::ChangeSceneAsync(scenePath);
        },
        "get_scene_name", []() {
            return SceneManager::GetSceneName();
        }
    );
}