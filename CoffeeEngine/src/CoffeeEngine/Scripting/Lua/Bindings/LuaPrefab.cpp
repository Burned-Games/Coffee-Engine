#include "LuaPrefab.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scene/Prefab.h"
#include "CoffeeEngine/Project/Project.h"

namespace Coffee {
    void RegisterPrefabBindings(sol::state& luaState)
    {
        // Helper function to load and instantiate a prefab in one call
        luaState.set_function("instantiate_prefab", [](const std::string& path, sol::optional<glm::mat4> transform) -> Entity {
            auto scene = SceneManager::GetActiveScene().get();
            if (!scene) {
                COFFEE_CORE_ERROR("Lua: Cannot instantiate prefab: no active scene");
                return Entity();
            }

            // Resolve the path (relative to project directory if not absolute)
            std::string resolvedPath = path;
            if (!path.empty() && path[0] != '/') {
                auto projectDir = Project::GetActive()->GetProjectDirectory();
                resolvedPath = (projectDir / path).string();
            }

            auto prefab = Prefab::Load(resolvedPath);
            if (!prefab) {
                COFFEE_CORE_ERROR("Lua: Failed to load prefab: {0} (resolved to {1})", path, resolvedPath);
                return Entity();
            }

            Entity prefabEntity = prefab->Instantiate(scene, transform.value_or(glm::mat4(1.0f)));

            std::function<void(Entity&)> markAudioComponentsToDelete = [&](Entity& entity) {
                if (entity.HasComponent<AudioSourceComponent>())
                {
                    auto& audioSource = entity.GetComponent<AudioSourceComponent>();
                    audioSource.toUnregister = true;
                }

                if (entity.HasComponent<AudioListenerComponent>())
                {
                    auto& audioListener = entity.GetComponent<AudioSourceComponent>();
                    audioListener.toUnregister = true;
                }

                if constexpr (requires(Entity e) { e.GetChildren(); })
                {
                    for (auto& child : entity.GetChildren())
                    {
                        markAudioComponentsToDelete(child);
                    }
                }
            };

            markAudioComponentsToDelete(prefabEntity);

            return prefabEntity;
        });
    }
}