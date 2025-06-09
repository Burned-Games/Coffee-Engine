#include "LuaEntity.h"

#include "CoffeeEngine/Scene/Entity.h"

void Coffee::RegisterEntityBindings(sol::state& luaState)
{
    luaState.new_usertype<Entity>("Entity",
            sol::constructors<Entity(), Entity(entt::entity, Scene*)>(),
            "get_component", [&luaState](Entity* self, const std::string& componentName) -> sol::object {
                if (componentName == "TagComponent") {
                    if (auto* component = self->TryGetComponent<TagComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a TagComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "TransformComponent") {
                    if (auto* component = self->TryGetComponent<TransformComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a TransformComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "CameraComponent") {
                    if (auto* component = self->TryGetComponent<CameraComponent>())
                        return sol::make_object(luaState, component);


                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a CameraComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "MeshComponent") {
                    if (auto* component = self->TryGetComponent<MeshComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a MeshComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "MaterialComponent") {
                    if (auto* component = self->TryGetComponent<MaterialComponent>())
                        return sol::make_object(luaState, component);
                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a MaterialComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "LightComponent") {
                    if (auto* component = self->TryGetComponent<LightComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a LightComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "ScriptComponent") {
                    if (auto* component = self->TryGetComponent<ScriptComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a ScriptComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "ParticlesSystemComponent") {
                    if (auto* component = self->TryGetComponent<ParticlesSystemComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a ParticlesSystemComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                }
                else if (componentName == "SpriteComponent")
                {
                    if (auto* component = self->TryGetComponent<SpriteComponent>())
                        return sol::make_object(luaState, std::ref(*component));

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a SpriteComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                }
                else if (componentName == "NavigationAgentComponent")
                {
                    if (auto* component = self->TryGetComponent<NavigationAgentComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a NavigationAgentComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "RigidbodyComponent") {
                    if (auto* component = self->TryGetComponent<RigidbodyComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a RigidbodyComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "AudioSourceComponent") {
                    if (auto* component = self->TryGetComponent<AudioSourceComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a AudioSourceComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "AnimatorComponent") {
                    if (auto* component = self->TryGetComponent<AnimatorComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a AnimatorComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "UIImageComponent") {
                    if (auto* component = self->TryGetComponent<UIImageComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a UIImageComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "UITextComponent") {
                    if (auto* component = self->TryGetComponent<UITextComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a UITextComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "UIToggleComponent") {
                    if (auto* component = self->TryGetComponent<UIToggleComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a UIToggleComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "UIButtonComponent") {
                    if (auto* component = self->TryGetComponent<UIButtonComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a UIButtonComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                } else if (componentName == "UISliderComponent") {
                    if (auto* component = self->TryGetComponent<UISliderComponent>())
                        return sol::make_object(luaState, component);

                    COFFEE_CORE_ERROR("Lua: Entity {} does not have a UISliderComponent",
                        self->TryGetComponent<TagComponent>() ? self->GetComponent<TagComponent>().Tag : "Unknown");
                    return sol::nil;
                }

                return sol::nil;
            },
            "has_component", [](Entity* self, const std::string& componentName) -> bool {
                if (componentName == "TagComponent") {
                    return self->HasComponent<TagComponent>();
                } else if (componentName == "TransformComponent") {
                    return self->HasComponent<TransformComponent>();
                } else if (componentName == "CameraComponent") {
                    return self->HasComponent<CameraComponent>();
                } else if (componentName == "MeshComponent") {
                    return self->HasComponent<MeshComponent>();
                } else if (componentName == "MaterialComponent") {
                    return self->HasComponent<MaterialComponent>();
                } else if (componentName == "LightComponent") {
                    return self->HasComponent<LightComponent>();
                } else if (componentName == "ScriptComponent") {
                    return self->HasComponent<ScriptComponent>();
                } else if (componentName == "ParticlesSystemComponent") {
                    return self->HasComponent<ParticlesSystemComponent>();
                } else if (componentName == "NavigationAgentComponent") {
                    return self->HasComponent<NavigationAgentComponent>();
                } else if (componentName == "RigidbodyComponent") {
                    return self->HasComponent<RigidbodyComponent>();
                } else if (componentName == "AnimatorComponent") {
                    return self->HasComponent<AnimatorComponent>();
                } else if (componentName == "AudioSourceComponent") {
                    return self->HasComponent<AudioSourceComponent>();
                } else if (componentName == "UIImageComponent") {
                    return self->HasComponent<UIImageComponent>();
                } else if (componentName == "UITextComponent") {
                    return self->HasComponent<UITextComponent>();
                } else if (componentName == "UIToggleComponent") {
                    return self->HasComponent<UIToggleComponent>();
                } else if (componentName == "UIButtonComponent") {
                    return self->HasComponent<UIButtonComponent>();
                } else if (componentName == "UISliderComponent") {
                    return self->HasComponent<UISliderComponent>();
                }
                return false;
            },
            "remove_component", [](Entity* self, const std::string& componentName) {
                if (componentName == "TagComponent") {
                    self->RemoveComponent<TagComponent>();
                } else if (componentName == "TransformComponent") {
                    self->RemoveComponent<TransformComponent>();
                } else if (componentName == "CameraComponent") {
                    self->RemoveComponent<CameraComponent>();
                } else if (componentName == "MeshComponent") {
                    self->RemoveComponent<MeshComponent>();
                } else if (componentName == "MaterialComponent") {
                    self->RemoveComponent<MaterialComponent>();
                } else if (componentName == "LightComponent") {
                    self->RemoveComponent<LightComponent>();
                } else if (componentName == "ScriptComponent") {
                    self->RemoveComponent<ScriptComponent>();
                } else if (componentName == "ParticlesSystemComponent") {
                    self->RemoveComponent<ParticlesSystemComponent>();
                } else if (componentName == "RigidbodyComponent") {
                    self->RemoveComponent<RigidbodyComponent>();
                } else if (componentName == "AudioSourceComponent") {
                    self->RemoveComponent<AudioSourceComponent>();
                }
            },
            "set_parent", &Entity::SetParent,
            "get_parent", &Entity::GetParent,
            "get_next_sibling", &Entity::GetNextSibling,
            "get_prev_sibling", &Entity::GetPrevSibling,
            "get_child", &Entity::GetChild,
            "get_children", &Entity::GetChildren,
            "is_valid", [](Entity* self) { return static_cast<bool>(*self); },
            "is_active", &Entity::IsActive,
            "set_active", &Entity::SetActive
        );
}