#include "LuaResources.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceUtils.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Shader.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Renderer/Mesh.h"
#include <sol/property.hpp>

namespace Coffee
{
    void RegisterResourcesBindings(sol::state& luaState)
    {
        // ResourceType enum binding
        luaState.new_enum("ResourceType",
            "Unknown", ResourceType::Unknown,
            "Texture", ResourceType::Texture,
            "Texture2D", ResourceType::Texture2D,
            "Cubemap", ResourceType::Cubemap,
            "Model", ResourceType::Model,
            "Mesh", ResourceType::Mesh,
            "Shader", ResourceType::Shader,
            "Material", ResourceType::Material,
            "PBRMaterial", ResourceType::PBRMaterial,
            "ShaderMaterial", ResourceType::ShaderMaterial,
            "AnimationSystem", ResourceType::AnimationSystem,
            "Skeleton", ResourceType::Skeleton,
            "Animation", ResourceType::Animation,
            "AnimationController", ResourceType::AnimationController,
            "Prefab", ResourceType::Prefab
        );

        // Base Resource class binding
        luaState.new_usertype<Resource>("Resource",
            "get_name", &Resource::GetName,
            "get_uuid", &Resource::GetUUID,
            "get_type", &Resource::GetType,
            "get_path", [](Resource& resource) { return resource.GetPath().string(); }
        );

        // Shader class binding (inherits from Resource)
        luaState.new_usertype<Shader>("Shader",
            sol::base_classes, sol::bases<Resource>()
    /*         "setBool", &Shader::setBool,
            "setInt", &Shader::setInt,
            "setFloat", &Shader::setFloat,
            "setVec2", &Shader::setVec2,
            "setVec3", &Shader::setVec3,
            "setVec4", &Shader::setVec4,
            "setMat2", &Shader::setMat2,
            "setMat3", &Shader::setMat3,
            "setMat4", &Shader::setMat4 */
        );

        // Model class binding (inherits from Resource)
    /*     luaState.new_usertype<Model>("Model",
            sol::base_classes, sol::bases<Resource>(),
            "GetMeshes", &Model::GetMeshes,
            "AddMesh", &Model::AddMesh,
            "GetNodeName", &Model::GetNodeName,
            "GetParent", &Model::GetParent,
            "GetChildren", &Model::GetChildren,
            "GetTransform", &Model::GetTransform,
            "HasAnimations", &Model::HasAnimations,
            "GetSkeleton", &Model::GetSkeleton,
            "GetAnimationController", &Model::GetAnimationController
        ); */

        // MaterialRenderSettings struct binding

        // enu, TransparencyMode, CullMode, BlendMode
/*         luaState.new_usertype<MaterialRenderSettings>("MaterialRenderSettings",
            "transparencyMode", &MaterialRenderSettings::transparencyMode,
            "alphaCutoff", &MaterialRenderSettings::alphaCutoff,
            "blendMode", &MaterialRenderSettings::blendMode,
            "cullMode", &MaterialRenderSettings::cullMode,
            "depthTest", &MaterialRenderSettings::depthTest,
            "wireframe", &MaterialRenderSettings::wireframe
        ); */
        luaState.new_enum("TransparencyMode",
            "Disabled", MaterialRenderSettings::TransparencyMode::Disabled,
            "Alpha", MaterialRenderSettings::TransparencyMode::Alpha,
            "AlphaBlend", MaterialRenderSettings::TransparencyMode::AlphaCutoff
        );
        luaState.new_enum("CullMode",
            "Front", MaterialRenderSettings::CullMode::Front,
            "Back", MaterialRenderSettings::CullMode::Back,
            "None", MaterialRenderSettings::CullMode::None
        );

        luaState.new_enum("BlendMode",
            "Mix", MaterialRenderSettings::BlendMode::Mix,
            "Add", MaterialRenderSettings::BlendMode::Add,
            "Subtract", MaterialRenderSettings::BlendMode::Subtract,
            "Multiply", MaterialRenderSettings::BlendMode::Multiply
        );

        // Material class binding (inherits from Resource)
        luaState.new_usertype<Material>("Material",
            sol::base_classes, sol::bases<Resource>(),
            "shader", sol::property(&Material::GetShader/* , &Material::SetShader */), // TODO: Move the SetShader to Material class
            "transparency_mode", sol::property(
                [](Material& material) { return material.GetRenderSettings().transparencyMode; },
                [](Material& material, MaterialRenderSettings::TransparencyMode mode) { material.GetRenderSettings().transparencyMode = mode; }
            ),
            "alpha_cutoff", sol::property(
                [](Material& material) { return material.GetRenderSettings().alphaCutoff; },
                [](Material& material, float cutoff) { material.GetRenderSettings().alphaCutoff = cutoff; }
            ),
            "blend_mode", sol::property(
                [](Material& material) { return material.GetRenderSettings().blendMode; },
                [](Material& material, MaterialRenderSettings::BlendMode mode) { material.GetRenderSettings().blendMode = mode; }
            ),
            "cull_mode", sol::property(
                [](Material& material) { return material.GetRenderSettings().cullMode; },
                [](Material& material, MaterialRenderSettings::CullMode mode) { material.GetRenderSettings().cullMode = mode; }
            ),
            "depth_test", sol::property(
                [](Material& material) { return material.GetRenderSettings().depthTest; },
                [](Material& material, bool depthTest) { material.GetRenderSettings().depthTest = depthTest; }
            ),
            "wireframe", sol::property(
                [](Material& material) { return material.GetRenderSettings().wireframe; },
                [](Material& material, bool wireframe) { material.GetRenderSettings().wireframe = wireframe; }
            )
        );

        // PBRMaterial class binding (inherits from Material)
        luaState.new_usertype<PBRMaterial>("PBRMaterial",
            sol::base_classes, sol::bases<Material>(),
            "color", sol::property(
                [](PBRMaterial& material) { return material.GetProperties().color; },
                [](PBRMaterial& material, const glm::vec4& color) { material.GetProperties().color = color; }
            ),
            "metalness", sol::property(
                [](PBRMaterial& material) { return material.GetProperties().metallic; },
                [](PBRMaterial& material, float metalness) { material.GetProperties().metallic = metalness; }
            ),
            "roughness", sol::property(
                [](PBRMaterial& material) { return material.GetProperties().roughness; },
                [](PBRMaterial& material, float roughness) { material.GetProperties().roughness = roughness; }
            ),
            "ao", sol::property(
                [](PBRMaterial& material) { return material.GetProperties().ao; },
                [](PBRMaterial& material, float ao) { material.GetProperties().ao = ao; }
            ),
            "emission", sol::property(
                [](PBRMaterial& material) { return material.GetProperties().emissive; },
                [](PBRMaterial& material, const glm::vec4& emission) { material.GetProperties().emissive = emission; }
            ),
            "albedo_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().albedo; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().albedo = texture ? texture.value() : nullptr;
                }
            ),
            "normal_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().normal; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().normal = texture ? texture.value() : nullptr;
                }
            ),
            "metallic_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().metallic; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().metallic = texture ? texture.value() : nullptr;
                }
            ),
            "roughness_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().roughness; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().roughness = texture ? texture.value() : nullptr;
                }
            ),
            "ao_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().ao; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().ao = texture ? texture.value() : nullptr;
                }
            ),
            "emission_texture", sol::property(
                [](PBRMaterial& material) { return material.GetTextures().emissive; },
                [](PBRMaterial& material, sol::optional<Ref<Texture2D>> texture) {
                    material.GetTextures().emissive = texture ? texture.value() : nullptr;
                }
            ),
            "new", sol::factories([](const std::string& name, sol::optional<Coffee::PBRMaterialTextures> textures) {
                if (textures) {
                    return Coffee::PBRMaterial::Create(name, &textures.value());
                } else {
                    return Coffee::PBRMaterial::Create(name, nullptr);
                }
            })
        );

        // ShaderMaterial class binding (inherits from Material)
        luaState.new_usertype<ShaderMaterial>("ShaderMaterial",
            sol::base_classes, sol::bases<Material>(),
            "new", sol::factories([](const std::string& name) {
                return Coffee::ShaderMaterial::Create(name, nullptr);
            })
        );

        // Mesh class binding (inherits from Resource)
/*         luaState.new_usertype<Mesh>("Mesh",
            sol::base_classes, sol::bases<Resource>()
        ); */

        // Texture class binding (inherits from Resource)
        luaState.new_usertype<Texture>("Texture",
            sol::base_classes, sol::bases<Resource>(),
            "width", &Texture::GetWidth,
            "height", &Texture::GetHeight,
            "image_format", &Texture::GetImageFormat
        );

        // Texture2D class binding (inherits from Texture)
        luaState.new_usertype<Texture2D>("Texture2D",
            sol::base_classes, sol::bases<Texture>(),
            "resize", &Texture2D::Resize,
            "clear", &Texture2D::Clear,
            "set_data", &Texture2D::SetData
        );

        // Cubemap class binding (inherits from Texture)
        luaState.new_usertype<Cubemap>("Cubemap",
            sol::base_classes, sol::bases<Texture>()
        );
    }

    void RegisterResourceLoadingBindings(sol::state& luaState)
    {
        luaState.set_function("load", [&](const std::string& path) -> sol::object
        {
            std::filesystem::path filePath = Project::GetActive()->GetProjectDirectory() / path;
            ResourceType type = GetResourceTypeFromExtension(filePath);
    
            switch (type)
            {
                case ResourceType::Texture2D:
                {
                    return sol::make_object(luaState, ResourceLoader::Load<Texture2D>(filePath));
                }
                case ResourceType::Cubemap:
                {
                    return sol::make_object(luaState, ResourceLoader::Load<Cubemap>(filePath));
                }
                case ResourceType::Shader:
                {
                    return sol::make_object(luaState, ResourceLoader::Load<Shader>(filePath));
                }
                case ResourceType::ShaderMaterial:
                {
                    return sol::make_object(luaState, ResourceLoader::Load<ShaderMaterial>(filePath));
                }
                case ResourceType::PBRMaterial:
                {
                    return sol::make_object(luaState, ResourceLoader::Load<PBRMaterial>(filePath));
                }
            }
        });
    }
}
