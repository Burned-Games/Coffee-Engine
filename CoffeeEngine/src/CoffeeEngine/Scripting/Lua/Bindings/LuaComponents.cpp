#include "LuaComponents.h"

#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scripting/GameSaver.h"
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include "CoffeeEngine/UI/UIManager.h"
#include <memory>

void Coffee::RegisterComponentsBindings(sol::state& luaState)
{
    luaState.new_usertype<TagComponent>("TagComponent",
        sol::constructors<TagComponent(), TagComponent(const std::string&)>(),
        "tag", &TagComponent::Tag
    );

    luaState.new_usertype<TransformComponent>("TransformComponent",
        sol::constructors<TransformComponent(), TransformComponent(const glm::vec3&)>(),
        "position", sol::property(&TransformComponent::GetLocalPosition, &TransformComponent::SetLocalPosition),
        "rotation", sol::property(&TransformComponent::GetLocalRotation, &TransformComponent::SetLocalRotation),
        "scale", sol::property(&TransformComponent::GetLocalScale, &TransformComponent::SetLocalScale),
        "get_local_transform", &TransformComponent::GetLocalTransform,
        "set_local_transform", &TransformComponent::SetLocalTransform,
        "get_world_transform", &TransformComponent::GetWorldTransform,
        "set_world_transform", &TransformComponent::SetWorldTransform
    );

    luaState.new_usertype<CameraComponent>("CameraComponent",
        sol::constructors<CameraComponent()>(),
        "camera", &CameraComponent::Camera
    );

    luaState.new_usertype<MeshComponent>("MeshComponent",
        sol::constructors<MeshComponent(), MeshComponent(Ref<Mesh>)>(),
        "mesh", &MeshComponent::mesh,
        "drawAABB", &MeshComponent::drawAABB,
        "get_mesh", &MeshComponent::GetMesh
    );

    luaState.new_usertype<MaterialComponent>("MaterialComponent",
        sol::constructors<MaterialComponent(), MaterialComponent(Ref<Material>)>(),
        "material", sol::property(
            [&](MaterialComponent& self) -> sol::object {
            if (!self.material) return sol::nil;
            switch (self.material->GetType())
            {
                case ResourceType::ShaderMaterial:
                    {
                        std::weak_ptr<ShaderMaterial> weakPtr = std::dynamic_pointer_cast<ShaderMaterial>(self.material);
                        return sol::make_object(luaState, weakPtr.lock());
                    }
                case ResourceType::PBRMaterial:
                    {
                        std::weak_ptr<PBRMaterial> weakPtr = std::dynamic_pointer_cast<PBRMaterial>(self.material);
                        return sol::make_object(luaState, weakPtr.lock());
                    }
                default:
                    return sol::nil;
            }
        },
        [](MaterialComponent& self, sol::object material) {
            if (material.is<Ref<ShaderMaterial>>())
                self.material = material.as<Ref<ShaderMaterial>>();
            else if (material.is<Ref<PBRMaterial>>())
                self.material = material.as<Ref<PBRMaterial>>();
            else
                COFFEE_CORE_ERROR("Lua: MaterialComponent can only be assigned a ShaderMaterial or PBRMaterial");
        })
    );

    luaState.new_usertype<LightComponent>("LightComponent",
        sol::constructors<LightComponent()>(),
        "color", &LightComponent::Color,
        "direction", &LightComponent::Direction,
        "position", &LightComponent::Position,
        "range", &LightComponent::Range,
        "attenuation", &LightComponent::Attenuation,
        "intensity", &LightComponent::Intensity,
        "angle", &LightComponent::Angle,
        "type", &LightComponent::type
    );

    luaState.new_usertype<NavigationAgentComponent>("NavigationAgentComponent",
        sol::constructors<NavigationAgentComponent()>(),
        "path", &NavigationAgentComponent::Path,
        "find_path", &NavigationAgentComponent::FindPath
    );

    luaState.new_usertype<ScriptComponent>("ScriptComponent",
        sol::constructors<ScriptComponent(), ScriptComponent(const std::filesystem::path& path, ScriptingLanguage language)>(),
        sol::meta_function::index, [](ScriptComponent& self, const std::string& key) {
            return std::dynamic_pointer_cast<LuaScript>(self.script)->GetVariable<sol::object>(key);
        },
        sol::meta_function::new_index, [](ScriptComponent& self, const std::string& key, sol::object value) {
            std::dynamic_pointer_cast<LuaScript>(self.script)->SetVariable(key, value);
        },
        sol::meta_function::call, [](ScriptComponent& self, const std::string& functionName, sol::variadic_args args) {
            std::dynamic_pointer_cast<LuaScript>(self.script)->CallFunction(functionName);
        }
    );


    luaState.new_usertype<ParticlesSystemComponent>("ParticlesSystemComponent", sol::constructors<ParticlesSystemComponent()>(),
        "emit",&ParticlesSystemComponent::Emit,
        "set_looping",&ParticlesSystemComponent::SetLooping,
        "get_emitter", &ParticlesSystemComponent::GetParticleEmitter
        );

    luaState.new_usertype<SpriteComponent>("SpriteComponent", sol::constructors<SpriteComponent()>(),
        "tint_color", sol::property(&SpriteComponent::GetTintColor, &SpriteComponent::SetTintColor));

    luaState.new_usertype<RigidbodyComponent>("RigidbodyComponent",
        "rb", &RigidbodyComponent::rb,
        "on_collision_enter", [](RigidbodyComponent& self, sol::protected_function fn) {
            self.callback.OnCollisionEnter([fn](CollisionInfo& info) {
                fn(info.entityA, info.entityB);
            });
        },
        "on_collision_stay", [](RigidbodyComponent& self, sol::protected_function fn) {
            self.callback.OnCollisionStay([fn](CollisionInfo& info) {
                fn(info.entityA, info.entityB);
            });
        },
        "on_collision_exit", [](RigidbodyComponent& self, sol::protected_function fn) {
            self.callback.OnCollisionExit([fn](CollisionInfo& info) {
                fn(info.entityA, info.entityB);
            });
        }
    );

    luaState.new_usertype<AnimatorComponent>(
        "AnimatorComponent", sol::constructors<AnimatorComponent(), AnimatorComponent()>(),
        "set_current_animation", &AnimatorComponent::SetCurrentAnimation,
        "set_upper_animation", &AnimatorComponent::SetUpperAnimation,
        "set_lower_animation", &AnimatorComponent::SetLowerAnimation
    );

    luaState.new_usertype<AudioSourceComponent>("AudioSourceComponent",
    sol::constructors<AudioSourceComponent(), AudioSourceComponent()>(),
     "set_volume", &AudioSourceComponent::SetVolume,
     "play", &AudioSourceComponent::Play,
     "pause", &AudioSourceComponent::Stop);

    luaState.new_usertype<UIImageComponent>("UIImageComponent", sol::constructors<UIImageComponent()>(),
        "set_color", [](UIImageComponent& self, const glm::vec4& color) { self.Color = color; },
        "set_rect", [](UIImageComponent& self, const glm::vec4& uvRect) { self.UVRect = uvRect; }
    );

    luaState.new_usertype<UITextComponent>("UITextComponent",
        "set_text", [](UITextComponent& self, const std::string& text) { self.Text = text; },
        "set_color", [](UITextComponent& self, const glm::vec4& color) { self.Color = color; },
        "kerning", &UITextComponent::Kerning,
        "line_spacing", &UITextComponent::LineSpacing,
        "font_size", &UITextComponent::FontSize
    );

    luaState.new_usertype<UIToggleComponent>("UIToggleComponent",
        "value", &UIToggleComponent::Value
    );

    luaState.new_enum<UIButtonComponent::State>("State",
    {
        {"Normal", UIButtonComponent::State::Normal},
        {"Hover", UIButtonComponent::State::Hover},
        {"Pressed", UIButtonComponent::State::Pressed},
        {"Disabled", UIButtonComponent::State::Disabled}
    });

    luaState.new_usertype<UIButtonComponent>("UIButtonComponent",
        "interactable", &UIButtonComponent::Interactable,
        "state", &UIButtonComponent::CurrentState,
        "set_normal_color", [](UIButtonComponent& self, const glm::vec4& color) { self.NormalColor = color; },
        "set_hover_color", [](UIButtonComponent& self, const glm::vec4& color) { self.HoverColor = color; },
        "set_pressed_color", [](UIButtonComponent& self, const glm::vec4& color) { self.PressedColor = color; },
        "set_disabled_color", [](UIButtonComponent& self, const glm::vec4& color) { self.DisabledColor = color; }
    );

    luaState.new_usertype<UISliderComponent>("UISliderComponent",
        "value", &UISliderComponent::Value,
        "min_value", &UISliderComponent::MinValue,
        "max_value", &UISliderComponent::MaxValue,
        "selected", &UISliderComponent::Selected,
        "handle_scale", &UISliderComponent::HandleScale
    );

    luaState.set_function("save_progress", [](const std::string& key, const sol::object& value) {
        if (value.is<int>()) {
            GameSaver::GetInstance().SaveVariable(key, value.as<int>());
        } else if (value.is<float>()) {
            GameSaver::GetInstance().SaveVariable(key, value.as<float>());
        } else if (value.is<bool>()) {
            GameSaver::GetInstance().SaveVariable(key, value.as<bool>());
        }
        GameSaver::GetInstance().SaveToFile();
    });

    luaState.set_function("load_progress", [&luaState](const std::string& key, sol::object defaultValue) -> sol::object {
        GameSaver::GetInstance().LoadFromFile();

        GameSaver::SaveValue value;
        if (defaultValue.is<int>()) {
            value = GameSaver::GetInstance().LoadVariable(key, defaultValue.as<int>());
        } else if (defaultValue.is<float>()) {
            value = GameSaver::GetInstance().LoadVariable(key, defaultValue.as<float>());
        } else if (defaultValue.is<bool>()) {
            value = GameSaver::GetInstance().LoadVariable(key, defaultValue.as<bool>());
        } else {
            return defaultValue;
        }

        if (std::holds_alternative<int>(value)) {
            return sol::make_object(luaState, std::get<int>(value));
        } else if (std::holds_alternative<float>(value)) {
            return sol::make_object(luaState, std::get<float>(value));
        } else if (std::holds_alternative<bool>(value)) {
            return sol::make_object(luaState, std::get<bool>(value));
        }

        return defaultValue;
    });

    luaState.set_function("scale_ui_element", [](const Entity entity, float scaleX, sol::optional<float> optScaleY) {
        float scaleY = optScaleY.value_or(scaleX);
        UIManager::MarkToTransform(entity, glm::vec2(scaleX, scaleY));
    });

    luaState.set_function("move_ui_element", [](const Entity entity, float offsetX, float offsetY) {
        UIManager::MarkToTransform(entity, glm::vec2(offsetX, offsetY), true);
    });

    luaState.set_function("rotate_ui_element", [](const Entity entity, float angle) {
        UIManager::MarkToTransform(entity, angle);
    });
}