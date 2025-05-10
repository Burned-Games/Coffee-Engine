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
        "get_all_entities", &Scene::GetAllEntities,
        "debug_flags", sol::property(&Scene::GetDebugFlags)
    );
    
    luaState.new_usertype<SceneDebugFlags>("SceneDebugFlags",
        "debug_draw", sol::property(
            [](const SceneDebugFlags& self) { return self.DebugDraw; },
            [](SceneDebugFlags& self, const bool value) {
                self.DebugDraw = value;
                self.ShowNavMesh = value;
                self.ShowNavMeshPath = value;
                self.ShowColliders = value;
                self.ShowOctree = value;
            }
        ),
        "show_nav_mesh", &SceneDebugFlags::ShowNavMesh,
        "show_nav_mesh_path", &SceneDebugFlags::ShowNavMeshPath,
        "show_colliders", &SceneDebugFlags::ShowColliders,
        "show_octree", &SceneDebugFlags::ShowOctree
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