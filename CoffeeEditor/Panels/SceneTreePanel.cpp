#include "SceneTreePanel.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/FileDialog.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/Camera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "imgui_internal.h"
#include <IconsLucide.h>

#include <CoffeeEngine/Scripting/Script.h>
#include <any>
#include <array>
#include <cstdint>
#include <cstring>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <memory>
#include <string>

namespace Coffee
{

    SceneTreePanel::SceneTreePanel(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void SceneTreePanel::OnImGuiRender()
    {
        if (!m_Visible)
            return;

        ImGui::Begin("Scene Tree");

        // delete node and all children if supr is pressed and the node is selected
        if (ImGui::IsKeyPressed(ImGuiKey_Delete) && m_SelectionContext)
        {
            m_Context->DestroyEntity(m_SelectionContext);
            m_SelectionContext = {};
        }

        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D) && m_SelectionContext)
        {
            m_Context->Duplicate(m_SelectionContext);
        }

        //Button for adding entities to the scene tree
        if(ImGui::Button(ICON_LC_PLUS, {24,24}))
        {
            ImGui::OpenPopup("Add Entity...");
        }
        ShowCreateEntityMenu();
        ImGui::SameLine();

        static std::array<char, 256> searchBuffer;
        ImGui::InputTextWithHint("##searchbar", ICON_LC_SEARCH " Search by name:", searchBuffer.data(),
                                 searchBuffer.size());

        ImGui::BeginChild("entity tree", {0, 0}, ImGuiChildFlags_Border);

        auto view = m_Context->m_Registry.view<entt::entity>();
        for (auto entityID : view)
        {
            Entity entity{entityID, m_Context.get()};
            auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

            if (hierarchyComponent.m_Parent == entt::null)
            {
                DrawEntityNode(entity);
            }
        }

        ImGui::EndChild();

        // Entity Tree Drag and Drop functionality
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
            {
                const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                switch (resource->GetType())
                {
                case ResourceType::Model: {
                    const Ref<Model>& model = std::static_pointer_cast<Model>(resource);
                    AddModelToTheSceneTree(m_Context.get(), model);
                    break;
                }
                default:
                    break;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (ImGui::IsWindowHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            m_SelectionContext = {};
        }

        ImGui::End();

        ImGui::Begin("Inspector");
        if (m_SelectionContext)
        {
            DrawComponents(m_SelectionContext);
        }

        ImGui::End();
    }

    void SceneTreePanel::DrawEntityNode(Entity entity)
    {
        auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ((hierarchyComponent.m_First == entt::null) ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entityNameTag.c_str());

        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        // Code of Double clicking the item for changing the name (WIP)

        ImVec2 itemSize = ImGui::GetItemRectSize();

        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            ImVec2 popupPos = ImGui::GetItemRectMin();
            float indent = ImGui::GetStyle().IndentSpacing;
            ImGui::SetNextWindowPos({popupPos.x + indent, popupPos.y});
            ImGui::OpenPopup("EntityPopup");
        }

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

        if (ImGui::BeginPopup("EntityPopup" /*, ImGuiWindowFlags_NoBackground*/))
        {
            auto buff = entity.GetComponent<TagComponent>().Tag.c_str();
            ImGui::SetNextItemWidth(itemSize.x - ImGui::GetStyle().IndentSpacing);
            ImGui::InputText("##entity-name", (char*)buff, 128);
            ImGui::EndPopup();
        }

        ImGui::PopStyleVar();

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("ENTITY_NODE", &entity,
                                      sizeof(Entity)); // Use the entity ID or a pointer as payload
            ImGui::Text("%s", entityNameTag.c_str());
            ImGui::EndDragDropSource();
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
            {
                // Assuming payload is an Entity, but you need to cast and check appropriately
                Entity payloadEntity = *(const Entity*)payload->Data;
                // Process the drop, e.g., reparenting the entity in the hierarchy
                // This is where you would update the ECS or scene graph
                HierarchyComponent::Reparent(
                    m_Context->m_Registry, (entt::entity)payloadEntity,
                    entity); // I think is not necessary do the casting, it does it automatically;
            }
            ImGui::EndDragDropTarget();
        }

        if (opened)
        {
            if (hierarchyComponent.m_First != entt::null)
            {
                // Recursively draw all children
                Entity childEntity{hierarchyComponent.m_First, m_Context.get()};
                while ((entt::entity)childEntity != entt::null)
                {
                    DrawEntityNode(childEntity);
                    auto& childHierarchyComponent = childEntity.GetComponent<HierarchyComponent>();
                    childEntity = Entity{childHierarchyComponent.m_Next, m_Context.get()};
                }
            }
            ImGui::TreePop();
        }
    }

    void SceneTreePanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

            ImGui::Text(ICON_LC_TAG " Tag");
            ImGui::SameLine();

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, entityNameTag.c_str());

            if (ImGui::InputText("##", buffer, sizeof(buffer)))
            {
                entityNameTag = std::string(buffer);
            }

            ImGui::Separator();
        }

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transformComponent = entity.GetComponent<TransformComponent>();

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Position");
                ImGui::DragFloat3("##Position", glm::value_ptr(transformComponent.Position), 0.1f);

                ImGui::Text("Rotation");
                ImGui::DragFloat3("##Rotation", glm::value_ptr(transformComponent.Rotation), 0.1f);

                ImGui::Text("Scale");
                ImGui::DragFloat3("##Scale", glm::value_ptr(transformComponent.Scale), 0.1f);
            }
        }

        if (entity.HasComponent<CameraComponent>())
        {
            auto& cameraComponent = entity.GetComponent<CameraComponent>();
            SceneCamera& sceneCamera = cameraComponent.Camera;
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Camera", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Projection Type");
                if (ImGui::BeginCombo("##Projection Type",
                                      sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE
                                          ? "Perspective"
                                          : "Orthographic"))
                {
                    if (ImGui::Selectable("Perspective",
                                          sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::PERSPECTIVE);
                    }
                    if (ImGui::Selectable("Orthographic",
                                          sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC))
                    {
                        sceneCamera.SetProjectionType(Camera::ProjectionType::ORTHOGRAPHIC);
                    }
                    ImGui::EndCombo();
                }

                if (sceneCamera.GetProjectionType() == Camera::ProjectionType::PERSPECTIVE)
                {
                    ImGui::Text("Field of View");
                    float fov = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Field of View", &fov, 0.1f, 0.0f, 180.0f))
                    {
                        sceneCamera.SetFOV(fov);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if (sceneCamera.GetProjectionType() == Camera::ProjectionType::ORTHOGRAPHIC)
                {
                    ImGui::Text("Orthographic Size");
                    float orthoSize = sceneCamera.GetFOV();
                    if (ImGui::DragFloat("##Orthographic Size", &orthoSize, 0.1f))
                    {
                        sceneCamera.SetFOV(orthoSize);
                    }

                    ImGui::Text("Near Clip");
                    float nearClip = sceneCamera.GetNearClip();
                    if (ImGui::DragFloat("##Near Clip", &nearClip, 0.1f))
                    {
                        sceneCamera.SetNearClip(nearClip);
                    }

                    ImGui::Text("Far Clip");
                    float farClip = sceneCamera.GetFarClip();
                    if (ImGui::DragFloat("##Far Clip", &farClip, 0.1f))
                    {
                        sceneCamera.SetFarClip(farClip);
                    }
                }

                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<CameraComponent>();
                }
            }
        }

        if (entity.HasComponent<LightComponent>())
        {
            auto& lightComponent = entity.GetComponent<LightComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Light", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Light Type");
                ImGui::Combo("##Light Type", (int*)&lightComponent.type, "Directional\0Point\0Spot\0");

                ImGui::Text("Color");
                ImGui::ColorEdit3("##Color", glm::value_ptr(lightComponent.Color));

                ImGui::Text("Intensity");
                ImGui::DragFloat("##Intensity", &lightComponent.Intensity, 0.1f);

                if (lightComponent.type == LightComponent::Type::PointLight ||
                    lightComponent.type == LightComponent::Type::SpotLight)
                {
                    ImGui::Text("Range");
                    ImGui::DragFloat("##Range", &lightComponent.Range, 0.1f);
                }

                if (lightComponent.type == LightComponent::Type::PointLight)
                {
                    ImGui::Text("Attenuation");
                    ImGui::DragFloat("##Attenuation", &lightComponent.Attenuation, 0.1f);
                }
                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<LightComponent>();
                }
            }
        }

        if (entity.HasComponent<MeshComponent>())
        {
            auto& meshComponent = entity.GetComponent<MeshComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Mesh", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Mesh");
                ImGui::SameLine();
                const std::string& meshName =
                    meshComponent.GetMesh() ? meshComponent.GetMesh()->GetName() : "Missing Mesh!!";
                if (ImGui::Button(meshName.c_str(), {64, 32}))
                {
                    ImGui::OpenPopup("MeshPopup");
                }
                if (ImGui::BeginPopup("MeshPopup"))
                {
                    if (ImGui::MenuItem("Quad"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateQuad();
                    }
                    if (ImGui::MenuItem("Cube"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCube();
                    }
                    if (ImGui::MenuItem("Sphere"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateSphere();
                    }
                    if (ImGui::MenuItem("Plane"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreatePlane();
                    }
                    if (ImGui::MenuItem("Cylinder"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCylinder();
                    }
                    if (ImGui::MenuItem("Cone"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCone();
                    }
                    if (ImGui::MenuItem("Torus"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateTorus();
                    }
                    if (ImGui::MenuItem("Capsule"))
                    {
                        meshComponent.mesh = PrimitiveMesh::CreateCapsule();
                    }
                    if (ImGui::MenuItem("Save Mesh"))
                    {
                        COFFEE_ERROR("Save Mesh not implemented yet!");
                    }
                    ImGui::EndPopup();
                }
                ImGui::Checkbox("Draw AABB", &meshComponent.drawAABB);

                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<MeshComponent>();
                }
            }
        }

        if (entity.HasComponent<MaterialComponent>())
        {
            // Move this function to another site
            auto DrawTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture) {
                auto& materialComponent = entity.GetComponent<MaterialComponent>();
                uint32_t textureID = texture ? texture->GetID() : 0;
                ImGui::ImageButton(label.c_str(), (ImTextureID)textureID, {64, 64});

                auto textureImageFormat = [](ImageFormat format) -> std::string {
                    switch (format)
                    {
                    case ImageFormat::R8:
                        return "R8";
                    case ImageFormat::RGB8:
                        return "RGB8";
                    case ImageFormat::RGBA8:
                        return "RGBA8";
                    case ImageFormat::SRGB8:
                        return "SRGB8";
                    case ImageFormat::SRGBA8:
                        return "SRGBA8";
                    case ImageFormat::RGBA32F:
                        return "RGBA32F";
                    case ImageFormat::DEPTH24STENCIL8:
                        return "DEPTH24STENCIL8";
                    }
                };

                if (ImGui::IsItemHovered() and texture)
                {
                    ImGui::SetTooltip("Name: %s\nSize: %d x %d\nPath: %s", texture->GetName().c_str(),
                                      texture->GetWidth(), texture->GetHeight(),
                                      textureImageFormat(texture->GetImageFormat()).c_str(),
                                      texture->GetPath().c_str());
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                    {
                        const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                        if (resource->GetType() == ResourceType::Texture2D)
                        {
                            const Ref<Texture2D>& t = std::static_pointer_cast<Texture2D>(resource);
                            texture = t;
                        }
                    }
                    ImGui::EndDragDropTarget();
                }

                ImGui::SameLine();
                if (ImGui::BeginCombo((label + "texture").c_str(), "", ImGuiComboFlags_NoPreview))
                {
                    if (ImGui::Selectable("Clear"))
                    {
                        texture = nullptr;
                    }
                    if (ImGui::Selectable("Open"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            Ref<Texture2D> t = Texture2D::Load(path);
                            texture = t;
                        }
                    }
                    ImGui::EndCombo();
                }
            };
            auto DrawCustomColorEdit4 = [&](const std::string& label, glm::vec4& color,
                                            const glm::vec2& size = {100, 32}) {
                // ImGui::ColorEdit4("##Albedo Color", glm::value_ptr(materialProperties.color),
                // ImGuiColorEditFlags_NoInputs);
                if (ImGui::ColorButton(label.c_str(), ImVec4(color.r, color.g, color.b, color.a), NULL,
                                       {size.x, size.y}))
                {
                    ImGui::OpenPopup("AlbedoColorPopup");
                }
                if (ImGui::BeginPopup("AlbedoColorPopup"))
                {
                    ImGui::ColorPicker4((label + "Picker").c_str(), glm::value_ptr(color),
                                        ImGuiColorEditFlags_NoInputs);
                    ImGui::EndPopup();
                }
            };

            auto& materialComponent = entity.GetComponent<MaterialComponent>();
            bool isCollapsingHeaderOpen = true;
            if (!materialComponent.material)
            {
                if (ImGui::CollapsingHeader("Material (Missing)", &isCollapsingHeaderOpen,
                                            ImGuiTreeNodeFlags_DefaultOpen))
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Material is missing or invalid!");

                    if (!isCollapsingHeaderOpen)
                    {
                        entity.RemoveComponent<MaterialComponent>();
                    }
                }
            }
            else
            {
                if (ImGui::CollapsingHeader("Material", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
                {
                    MaterialTextures& materialTextures = materialComponent.material->GetMaterialTextures();
                    MaterialProperties& materialProperties = materialComponent.material->GetMaterialProperties();

                    if (ImGui::TreeNode("Albedo"))
                    {
                        ImGui::BeginChild("##Albedo Child", {0, 0},
                                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

                        ImGui::Text("Color");
                        DrawCustomColorEdit4("##Albedo Color", materialProperties.color);

                        ImGui::Text("Texture");
                        DrawTextureWidget("##Albedo", materialTextures.albedo);

                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Metallic"))
                    {
                        ImGui::BeginChild("##Metallic Child", {0, 0},
                                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        ImGui::Text("Metallic");
                        ImGui::SliderFloat("##Metallic Slider", &materialProperties.metallic, 0.0f, 1.0f);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##Metallic", materialTextures.metallic);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Roughness"))
                    {
                        ImGui::BeginChild("##Roughness Child", {0, 0},
                                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        ImGui::Text("Roughness");
                        ImGui::SliderFloat("##Roughness Slider", &materialProperties.roughness, 0.1f, 1.0f);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##Roughness", materialTextures.roughness);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Emission"))
                    {
                        ImGui::BeginChild("##Emission Child", {0, 0},
                                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        // FIXME: Emissive color variable is local and do not affect the materialProperties.emissive!!
                        glm::vec4& emissiveColor = reinterpret_cast<glm::vec4&>(materialProperties.emissive);
                        emissiveColor.a = 1.0f;
                        DrawCustomColorEdit4("Color", emissiveColor);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##Emissive", materialTextures.emissive);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Normal Map"))
                    {
                        ImGui::BeginChild("##Normal Child", {0, 0},
                                          ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##Normal", materialTextures.normal);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Ambient Occlusion"))
                    {
                        ImGui::BeginChild("##AO Child", {0, 0}, ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        ImGui::Text("AO");
                        ImGui::SliderFloat("##AO Slider", &materialProperties.ao, 0.0f, 1.0f);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##AO", materialTextures.ao);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }

                    if (!isCollapsingHeaderOpen)
                    {
                        entity.RemoveComponent<MaterialComponent>();
                    }
                }
            }
        }

        if (entity.HasComponent<AudioSourceComponent>())
        {
            auto& audioSourceComponent = entity.GetComponent<AudioSourceComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Source", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (!Audio::audioBanks.empty() &&
                    ImGui::BeginCombo("Audio Bank", audioSourceComponent.audioBankName.c_str()))
                {
                    for (auto& bank : Audio::audioBanks)
                    {
                        const bool isSelected = (audioSourceComponent.audioBankName == bank->name);

                        if (bank->name != "Init" && ImGui::Selectable(bank->name.c_str()))
                        {
                            if (audioSourceComponent.audioBank != bank)
                            {
                                audioSourceComponent.audioBank = bank;
                                audioSourceComponent.audioBankName = bank->name;

                                if (!audioSourceComponent.eventName.empty())
                                {
                                    audioSourceComponent.eventName.clear();
                                    Audio::StopEvent(audioSourceComponent);
                                }
                            }
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                if (audioSourceComponent.audioBank &&
                    ImGui::BeginCombo("Audio Clip", audioSourceComponent.eventName.c_str()))
                {
                    for (const auto& event : audioSourceComponent.audioBank->events)
                    {
                        const bool isSelected = audioSourceComponent.eventName == event;

                        if (ImGui::Selectable(event.c_str()))
                        {
                            if (!audioSourceComponent.eventName.empty())
                                Audio::StopEvent(audioSourceComponent);

                            audioSourceComponent.eventName = event;
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }

                ImGui::Checkbox("Play On Awake", &audioSourceComponent.playOnAwake);

                if (ImGui::Checkbox("Mute", &audioSourceComponent.mute))
                    Audio::SetVolume(audioSourceComponent.gameObjectID,
                                     audioSourceComponent.mute ? 0.f : audioSourceComponent.volume);

                if (ImGui::SliderFloat("Volume", &audioSourceComponent.volume, 0.f, 1.f))
                {
                    if (audioSourceComponent.mute)
                        audioSourceComponent.mute = false;

                    Audio::SetVolume(audioSourceComponent.gameObjectID, audioSourceComponent.volume);
                }

                if (audioSourceComponent.audioBank && !audioSourceComponent.eventName.empty())
                {
                    if (!audioSourceComponent.isPlaying)
                    {
                        if (ImGui::SmallButton("Play"))
                        {
                            Audio::PlayEvent(audioSourceComponent);
                        }
                    }
                    else
                    {
                        if (!audioSourceComponent.isPaused)
                        {
                            if (ImGui::SmallButton("Pause"))
                            {
                                Audio::PauseEvent(audioSourceComponent);
                            }
                        }
                        else if (ImGui::SmallButton("Resume"))
                        {
                            Audio::ResumeEvent(audioSourceComponent);
                        }

                        ImGui::SameLine();

                        if (ImGui::SmallButton("Stop"))
                        {
                            Audio::StopEvent(audioSourceComponent);
                        }
                    }
                }
            }

            if (!isCollapsingHeaderOpen)
            {
                AudioZone::UnregisterObject(audioSourceComponent.gameObjectID);
                Audio::UnregisterAudioSourceComponent(audioSourceComponent);
                entity.RemoveComponent<AudioSourceComponent>();
            }
        }

        if (entity.HasComponent<AudioListenerComponent>())
        {
            auto& audioListenerComponent = entity.GetComponent<AudioListenerComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Listener", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
            }

            if (!isCollapsingHeaderOpen)
            {
                Audio::UnregisterAudioListenerComponent(audioListenerComponent);
                entity.RemoveComponent<AudioListenerComponent>();
            }
        }

        if (entity.HasComponent<AudioZoneComponent>())
        {
            auto& audioZoneComponent = entity.GetComponent<AudioZoneComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Audio Zone", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::BeginCombo("Bus Channels", audioZoneComponent.audioBusName.c_str()))
                {
                    for (auto& busName : AudioZone::busNames)
                    {
                        const bool isSelected = (audioZoneComponent.audioBusName == busName);

                        if (ImGui::Selectable(busName.c_str()))
                        {
                            if (audioZoneComponent.audioBusName != busName)
                            {
                                audioZoneComponent.audioBusName = busName;
                                AudioZone::UpdateReverbZone(audioZoneComponent);
                            }
                        }

                        if (isSelected)
                            ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text("Position");
                if (ImGui::DragFloat3("##ZonePosition", glm::value_ptr(audioZoneComponent.position), 0.1f) == true)
                    AudioZone::UpdateReverbZone(audioZoneComponent);

                ImGui::Text("Radius");
                if (ImGui::SliderFloat("##ZoneRadius", &audioZoneComponent.radius, 1.f, 100.f))
                    AudioZone::UpdateReverbZone(audioZoneComponent);
            }

            if (!isCollapsingHeaderOpen)
            {
                AudioZone::RemoveReverbZone(audioZoneComponent);
                entity.RemoveComponent<AudioZoneComponent>();
            }
        }

        // Add RigidBody component editor UI
       if (entity.HasComponent<RigidbodyComponent>())
        {
            auto& rbComponent = entity.GetComponent<RigidbodyComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Rigidbody", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (rbComponent.rb)
                {
                    // Rigidbody type
                    static const char* typeStrings[] = { "Static", "Dynamic", "Kinematic" };
                    int currentType = static_cast<int>(rbComponent.rb->GetBodyType());

                    ImGui::Text("Type");
                    if (ImGui::Combo("##Type", &currentType, typeStrings, IM_ARRAYSIZE(typeStrings)))
                    {
                        rbComponent.rb->SetBodyType(static_cast<RigidBody::Type>(currentType));
                    }

                    // Mass (only for dynamic bodies)
                    if (rbComponent.rb->GetBodyType() != RigidBody::Type::Static)
                    {
                        ImGui::Text("Mass");
                        float mass = rbComponent.rb->GetMass();
                        if (ImGui::DragFloat("##Mass", &mass, 0.1f, 0.001f, 1000.0f))
                        {
                            rbComponent.rb->SetMass(mass);
                        }

                        // Use gravity
                        ImGui::Text("Use Gravity");
                        bool useGravity = rbComponent.rb->GetUseGravity();
                        if (ImGui::Checkbox("##UseGravity", &useGravity))
                        {
                            rbComponent.rb->SetUseGravity(useGravity);
                        }
                    }

                    // Freeze axes
                    ImGui::Text("Freeze Position");
                    ImGui::Columns(3, "FreezePositionColumns", false);

                    // X Axis
                    bool freezeX = rbComponent.rb->GetFreezeX();
                    if (ImGui::Checkbox("X##FreezeX", &freezeX))
                    {
                        rbComponent.rb->SetFreezeX(freezeX);
                    }
                    ImGui::NextColumn();

                    // Y Axis
                    bool freezeY = rbComponent.rb->GetFreezeY();
                    if (ImGui::Checkbox("Y##FreezeY", &freezeY))
                    {
                        rbComponent.rb->SetFreezeY(freezeY);
                    }
                    ImGui::NextColumn();

                    // Z Axis
                    bool freezeZ = rbComponent.rb->GetFreezeZ();
                    if (ImGui::Checkbox("Z##FreezeZ", &freezeZ))
                    {
                        rbComponent.rb->SetFreezeZ(freezeZ);
                    }

                    ImGui::Columns(1);

                    // Freeze rotation axes
                    ImGui::Text("Freeze Rotation");
                    ImGui::Columns(3, "FreezeRotationColumns", false);

                    // X Rotation Axis
                    bool freezeRotX = rbComponent.rb->GetFreezeRotX();
                    if (ImGui::Checkbox("X##FreezeRotX", &freezeRotX))
                    {
                        rbComponent.rb->SetFreezeRotX(freezeRotX);
                    }
                    ImGui::NextColumn();

                    // Y Rotation Axis
                    bool freezeRotY = rbComponent.rb->GetFreezeRotY();
                    if (ImGui::Checkbox("Y##FreezeRotY", &freezeRotY))
                    {
                        rbComponent.rb->SetFreezeRotY(freezeRotY);
                    }
                    ImGui::NextColumn();

                    // Z Rotation Axis
                    bool freezeRotZ = rbComponent.rb->GetFreezeRotZ();
                    if (ImGui::Checkbox("Z##FreezeRotZ", &freezeRotZ))
                    {
                        rbComponent.rb->SetFreezeRotZ(freezeRotZ);
                    }

                    ImGui::Columns(1);

                    // Add collider type selection and configuration
                    ImGui::Separator();
                    ImGui::Text("Collider");

                    Ref<Collider> currentCollider = rbComponent.rb->GetCollider();
                    int colliderType = -1; // -1: Unknown, 0: Box, 1: Sphere, 2: Capsule

                    if (currentCollider) {
                        if (std::dynamic_pointer_cast<BoxCollider>(currentCollider)) {
                            colliderType = 0;
                        } else if (std::dynamic_pointer_cast<SphereCollider>(currentCollider)) {
                            colliderType = 1;
                        } else if (std::dynamic_pointer_cast<CapsuleCollider>(currentCollider)) {
                            colliderType = 2;
                        }
                    }

                    static const char* colliderTypeNames[] = { "Box", "Sphere", "Capsule" };
                    int newColliderType = colliderType;

                    if (ImGui::Combo("Type##ColliderType", &newColliderType, colliderTypeNames, 3)) {
                        // User selected a new collider type
                        Ref<Collider> newCollider;

                        // Create new collider based on selection
                        switch (newColliderType) {
                            case 0: { // Box
                                glm::vec3 size(1.0f, 1.0f, 1.0f);
                                if (auto boxCollider = std::dynamic_pointer_cast<BoxCollider>(currentCollider)) {
                                    size = boxCollider->GetSize();
                                }
                                newCollider = CreateRef<BoxCollider>(size);
                                break;
                            }
                            case 1: { // Sphere
                                float radius = 0.5f;
                                if (auto sphereCollider = std::dynamic_pointer_cast<SphereCollider>(currentCollider)) {
                                    radius = sphereCollider->GetRadius();
                                }
                                newCollider = CreateRef<SphereCollider>(radius);
                                break;
                            }
                            case 2: { // Capsule
                                float radius = 0.5f;
                                float height = 2.0f;
                                if (auto capsuleCollider = std::dynamic_pointer_cast<CapsuleCollider>(currentCollider)) {
                                    radius = capsuleCollider->GetRadius();
                                    height = capsuleCollider->GetHeight();
                                }
                                newCollider = CreateRef<CapsuleCollider>(radius, height);
                                break;
                            }
                        }

                        if (newCollider) {
                            // Store current rigidbody properties
                            RigidBody::Properties props = rbComponent.rb->GetProperties();
                            glm::vec3 position = rbComponent.rb->GetPosition();
                            glm::vec3 rotation = rbComponent.rb->GetRotation();

                            // Remove from physics world
                            m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                            // Create new rigidbody with new collider
                            rbComponent.rb = RigidBody::Create(props, newCollider);
                            rbComponent.rb->SetPosition(position);
                            rbComponent.rb->SetRotation(rotation);

                            // Add back to physics world
                            m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());

                            // Set user pointer for collision detection
                            rbComponent.rb->GetNativeBody()->setUserPointer(
                                reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                        }
                    }

                    // Show collider-specific properties
                    if (currentCollider) {
                        switch (colliderType) {
                            case 0: { // Box collider properties
                                auto boxCollider = std::dynamic_pointer_cast<BoxCollider>(currentCollider);
                                if (boxCollider) {
                                    glm::vec3 size = boxCollider->GetSize();

                                    ImGui::Text("Size");
                                    if (ImGui::DragFloat3("##BoxSize", glm::value_ptr(size), 0.1f, 0.01f, 100.0f)) {
                                        // Create new box collider with updated size
                                        Ref<BoxCollider> newCollider = CreateRef<BoxCollider>(size);

                                        // Store current rigidbody properties
                                        RigidBody::Properties props = rbComponent.rb->GetProperties();
                                        glm::vec3 position = rbComponent.rb->GetPosition();
                                        glm::vec3 rotation = rbComponent.rb->GetRotation();
                                        glm::vec3 velocity = rbComponent.rb->GetVelocity();

                                        // Remove from physics world
                                        m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                                        // Create new rigidbody with new collider
                                        rbComponent.rb = RigidBody::Create(props, newCollider);
                                        rbComponent.rb->SetPosition(position);
                                        rbComponent.rb->SetRotation(rotation);
                                        rbComponent.rb->SetVelocity(velocity);

                                        // Add back to physics world
                                        m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                                        rbComponent.rb->GetNativeBody()->setUserPointer(
                                            reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                                    }
                                }
                                break;
                            }
                            case 1: { // Sphere collider properties
                                auto sphereCollider = std::dynamic_pointer_cast<SphereCollider>(currentCollider);
                                if (sphereCollider) {
                                    float radius = sphereCollider->GetRadius();

                                    ImGui::Text("Radius");
                                    if (ImGui::DragFloat("##SphereRadius", &radius, 0.1f, 0.01f, 100.0f)) {
                                        // Create new sphere collider with updated radius
                                        Ref<Collider> newCollider = CreateRef<SphereCollider>(radius);

                                        // Store current rigidbody properties
                                        RigidBody::Properties props = rbComponent.rb->GetProperties();
                                        glm::vec3 position = rbComponent.rb->GetPosition();
                                        glm::vec3 rotation = rbComponent.rb->GetRotation();
                                        glm::vec3 velocity = rbComponent.rb->GetVelocity();

                                        // Remove from physics world
                                        m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                                        // Create new rigidbody with new collider
                                        rbComponent.rb = RigidBody::Create(props, newCollider);
                                        rbComponent.rb->SetPosition(position);
                                        rbComponent.rb->SetRotation(rotation);
                                        rbComponent.rb->SetVelocity(velocity);

                                        // Add back to physics world
                                        m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                                        rbComponent.rb->GetNativeBody()->setUserPointer(
                                            reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                                    }
                                }
                                break;
                            }
                            case 2: { // Capsule collider properties
                                auto capsuleCollider = std::dynamic_pointer_cast<CapsuleCollider>(currentCollider);
                                if (capsuleCollider) {
                                    float radius = capsuleCollider->GetRadius();
                                    float height = capsuleCollider->GetHeight();

                                    float totalHeight = height + 2.0f * radius; // Total height including spherical caps

                                    ImGui::Text("Radius");
                                    bool radiusChanged = ImGui::DragFloat("##CapsuleRadius", &radius, 0.1f, 0.01f, 100.0f);

                                    ImGui::Text("Total Height");
                                    bool heightChanged = ImGui::DragFloat("##CapsuleHeight", &totalHeight, 0.1f, 0.01f, 100.0f);

                                    if (radiusChanged || heightChanged) {
                                        if (totalHeight < radius * 2.0f) {
                                            totalHeight = radius * 2.0f;
                                        }

                                        float cylinderHeight = totalHeight - 2.0f * radius;

                                        // Create new capsule collider with updated parameters
                                        Ref<Collider> newCollider = CreateRef<CapsuleCollider>(radius, cylinderHeight);

                                        // Store current rigidbody properties
                                        RigidBody::Properties props = rbComponent.rb->GetProperties();
                                        glm::vec3 position = rbComponent.rb->GetPosition();
                                        glm::vec3 rotation = rbComponent.rb->GetRotation();
                                        glm::vec3 velocity = rbComponent.rb->GetVelocity();

                                        // Remove from physics world
                                        m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                                        // Create new rigidbody with new collider
                                        rbComponent.rb = RigidBody::Create(props, newCollider);
                                        rbComponent.rb->SetPosition(position);
                                        rbComponent.rb->SetRotation(rotation);
                                        rbComponent.rb->SetVelocity(velocity);

                                        // Add back to physics world
                                        m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                                        rbComponent.rb->GetNativeBody()->setUserPointer(
                                            reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                                    }
                                }
                                break;
                            }
                        }
                    }

                    // Add friction and drag controls
                    ImGui::Separator();

                    ImGui::Text("Friction");
                    float friction = rbComponent.rb->GetFriction();
                    if (ImGui::SliderFloat("##Friction", &friction, 0.0f, 1.0f))
                    {
                        rbComponent.rb->SetFriction(friction);
                    }

                    ImGui::Text("Linear Drag");
                    float linearDrag = rbComponent.rb->GetLinearDrag();
                    if (ImGui::SliderFloat("##LinearDrag", &linearDrag, 0.0f, 1.0f))
                    {
                        rbComponent.rb->SetLinearDrag(linearDrag);
                    }

                    ImGui::Text("Angular Drag");
                    float angularDrag = rbComponent.rb->GetAngularDrag();
                    if (ImGui::SliderFloat("##AngularDrag", &angularDrag, 0.0f, 1.0f))
                    {
                        rbComponent.rb->SetAngularDrag(angularDrag);
                    }

                    // Show current velocity
                    glm::vec3 velocity = rbComponent.rb->GetVelocity();
                    ImGui::Text("Current Velocity: X: %.2f, Y: %.2f, Z: %.2f",
                                velocity.x, velocity.y, velocity.z);

                    // ---------------------Physics Debug Controls---------------------
                    /*
                    // Add force/impulse controls
                    static glm::vec3 forceToApply = {0.0f, 0.0f, 0.0f};
                    ImGui::Separator();
                    ImGui::Text("Physics Controls");
                    ImGui::DragFloat3("Vector", glm::value_ptr(forceToApply), 0.1f);

                    // Force & Impulse buttons
                    if (ImGui::Button("Apply Force"))
                    {
                        rbComponent.rb->ApplyForce(forceToApply);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Apply Impulse"))
                    {
                        rbComponent.rb->ApplyImpulse(forceToApply);
                    }

                    // Velocity buttons
                    if (ImGui::Button("Set Velocity"))
                    {
                        rbComponent.rb->SetVelocity(forceToApply);
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Add Velocity"))
                    {
                        rbComponent.rb->AddVelocity(forceToApply);
                    }

                    // Reset velocity
                    if (ImGui::Button("Reset Velocity"))
                    {
                        rbComponent.rb->ResetVelocity();
                    }
                    */
                }
                else
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "RigidBody instance is null!");
                }
            }

            if (!isCollapsingHeaderOpen)
            {
                // Remove from physics world before removing component
                if (rbComponent.rb && rbComponent.rb->GetNativeBody())
                {
                    m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());
                }
                entity.RemoveComponent<RigidbodyComponent>();
            }
        }

        const char* anchorPoints[] = { "TopLeft", "TopCenter", "TopRight",
                                  "CenterLeft", "Center", "CenterRight",
                                  "BottomLeft", "BottomCenter", "BottomRight" };

        auto DrawAnchorPointCombo = [&](UIAnchorPosition& anchor) {
            int currentAnchor = static_cast<int>(anchor);
            if (ImGui::Combo("Anchor Point", &currentAnchor, anchorPoints, IM_ARRAYSIZE(anchorPoints)))
            {
                anchor = static_cast<UIAnchorPosition>(currentAnchor);
            }
        };

        if (entity.HasComponent<UIImageComponent>())
        {
            auto& uiImageComponent = entity.GetComponent<UIImageComponent>();
            bool isCollapsingHeaderOpen = true;

            DrawAnchorPointCombo(uiImageComponent.Anchor);

            if (ImGui::CollapsingHeader("UI Image", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Size");
                ImGui::DragFloat2("##Size", glm::value_ptr(uiImageComponent.Size), 0.1f);

                ImGui::Checkbox("Visible", &uiImageComponent.Visible);


                if (uiImageComponent.texture)
                {
                    ImGui::Text("Current Texture: %s", uiImageComponent.texture->GetPath().c_str());
                    ImGui::Image((void*)(intptr_t)uiImageComponent.texture->GetID(), ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("No Texture Selected");
                }


                if (ImGui::Button("Select Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        if (texture)
                        {
                            uiImageComponent.SetTexture(texture);
                        }
                    }
                }

                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<UIImageComponent>();
                }
            }
        }

        if (entity.HasComponent<UITextComponent>()) {
            auto& uiTextComponent = entity.GetComponent<UITextComponent>();
            bool isCollapsingHeaderOpen = true;

            DrawAnchorPointCombo(uiTextComponent.Anchor);

            if (ImGui::CollapsingHeader("UI Text", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen)) {
                // Show text content
                ImGui::Text("Text Content");
                char buffer[256];
                memset(buffer, 0, sizeof(buffer));
                strncpy(buffer, uiTextComponent.Text.c_str(), sizeof(buffer) - 1);

                if (ImGui::InputTextMultiline("##Text", buffer, sizeof(buffer))) {
                    uiTextComponent.Text = std::string(buffer);
                }

                // Select font with a button
                ImGui::Text("Font Path");
                ImGui::SameLine();
                ImGui::Text("%s", uiTextComponent.FontPath.c_str());

                if (ImGui::Button("Select Font")) {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty()) {
                        uiTextComponent.FontPath = path;
                        uiTextComponent.font = std::make_shared<Font>(path);
                    }
                }
                // If there is no font, use default
                if (!uiTextComponent.font) {
                    uiTextComponent.font = Font::GetDefault();
                }

                // size
                ImGui::Text("Font Size");
                ImGui::DragFloat("##FontSize", &uiTextComponent.FontSize, 0.1f, 5.0f, 100.0f);

                // color
                ImGui::Text("Text Color");
                ImGui::ColorEdit4("##TextColor", glm::value_ptr(uiTextComponent.Color));

                // alignment
                ImGui::Text("Text Alignment");
                const char* alignmentOptions[] = { "Left", "Center", "Right" };
                int currentAlignment = static_cast<int>(uiTextComponent.Alignment);
                if (ImGui::Combo("##TextAlignment", &currentAlignment, alignmentOptions, IM_ARRAYSIZE(alignmentOptions))) {
                    uiTextComponent.Alignment = static_cast<UITextAlignment>(currentAlignment);
                }

                // visibility
                ImGui::Checkbox("Visible", &uiTextComponent.Visible);
            }
        }

       if (entity.HasComponent<UISliderComponent>())
        {
            auto& uiSliderComponent = entity.GetComponent<UISliderComponent>();
            bool isCollapsingHeaderOpen = true;

            DrawAnchorPointCombo(uiSliderComponent.Anchor);

            if (ImGui::CollapsingHeader("UI Slider", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {

                ImGui::Text("Value");
                ImGui::SliderFloat("##SliderValue", &uiSliderComponent.Value, 0.0f, 1.0f);


                ImGui::Text("Size");
                ImGui::DragFloat2("##SliderSize", glm::value_ptr(uiSliderComponent.Size), 0.1f);


                ImGui::Text("Handle Size");
                ImGui::DragFloat2("##HandleSize", glm::value_ptr(uiSliderComponent.HandleSize), 0.1f);


                ImGui::Text("Bar Texture");
                if (uiSliderComponent.barTexture)
                {
                    ImGui::Text("Current Texture: %s", uiSliderComponent.barTexture->GetPath().c_str());
                    ImGui::Image((void*)(intptr_t)uiSliderComponent.barTexture->GetID(), ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("No Bar Texture Selected");
                }


                if (ImGui::Button("Select Bar Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        if (texture)
                        {
                            uiSliderComponent.SetBarTexture(texture);
                        }
                    }
                }


                ImGui::Text("Handle Texture");
                if (uiSliderComponent.handleTexture)
                {
                    ImGui::Text("Current Texture: %s", uiSliderComponent.handleTexture->GetPath().c_str());
                    ImGui::Image((void*)(intptr_t)uiSliderComponent.handleTexture->GetID(), ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("No Handle Texture Selected");
                }


                if (ImGui::Button("Select Handle Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        if (texture)
                        {
                            uiSliderComponent.SetHandleTexture(texture);
                        }
                    }
                }


                ImGui::Checkbox("Visible", &uiSliderComponent.Visible);


            }
        }

        if (entity.HasComponent<UIButtonComponent>())
        {
            auto& uiButtonComponent = entity.GetComponent<UIButtonComponent>();
            bool isCollapsingHeaderOpen = true;

            DrawAnchorPointCombo(uiButtonComponent.Anchor);

            if (ImGui::CollapsingHeader("UI Button", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {

                const char* currentStateName = "";
                switch (uiButtonComponent.currentState)
                {
                    case UIButtonComponent::ButtonState::Base:     currentStateName = "Base";     break;
                    case UIButtonComponent::ButtonState::Selected: currentStateName = "Selected"; break;
                    case UIButtonComponent::ButtonState::Pressed:  currentStateName = "Pressed";  break;
                }


                if (ImGui::BeginCombo("Current State", currentStateName))
                {
                    if (ImGui::Selectable("Base", uiButtonComponent.currentState == UIButtonComponent::ButtonState::Base))
                    {
                        uiButtonComponent.currentState = UIButtonComponent::ButtonState::Base;
                    }
                    if (ImGui::Selectable("Selected", uiButtonComponent.currentState == UIButtonComponent::ButtonState::Selected))
                    {
                        uiButtonComponent.currentState = UIButtonComponent::ButtonState::Selected;
                    }
                    if (ImGui::Selectable("Pressed", uiButtonComponent.currentState == UIButtonComponent::ButtonState::Pressed))
                    {
                        uiButtonComponent.currentState = UIButtonComponent::ButtonState::Pressed;
                    }

                    ImGui::EndCombo();
                }


                ImGui::Text("Base Texture");
                if (uiButtonComponent.baseTexture)
                {
                    ImGui::Image((void*)(intptr_t)uiButtonComponent.baseTexture->GetID(), ImVec2(64, 64));
                    ImGui::SameLine();
                    if (ImGui::Button("Change Base Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.baseTexture = Texture2D::Load(path);
                        }
                    }
                }
                else
                {
                    ImGui::Text("No Base Texture Selected");
                    ImGui::SameLine();
                    if (ImGui::Button("Select Base Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.baseTexture = Texture2D::Load(path);
                        }
                    }
                }


                ImGui::Text("Selected Texture");
                if (uiButtonComponent.selectedTexture)
                {
                    ImGui::Image((void*)(intptr_t)uiButtonComponent.selectedTexture->GetID(), ImVec2(64, 64));
                    ImGui::SameLine();
                    if (ImGui::Button("Change Selected Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.selectedTexture = Texture2D::Load(path);
                        }
                    }
                }
                else
                {
                    ImGui::Text("No Selected Texture Selected");
                    ImGui::SameLine();
                    if (ImGui::Button("Select Selected Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.selectedTexture = Texture2D::Load(path);
                        }
                    }
                }


                ImGui::Text("Pressed Texture");
                if (uiButtonComponent.pressedTexture)
                {
                    ImGui::Image((void*)(intptr_t)uiButtonComponent.pressedTexture->GetID(), ImVec2(64, 64));
                    ImGui::SameLine();
                    if (ImGui::Button("Change Pressed Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.pressedTexture = Texture2D::Load(path);
                        }
                    }
                }
                else
                {
                    ImGui::Text("No Pressed Texture Selected");
                    ImGui::SameLine();
                    if (ImGui::Button("Select Pressed Texture"))
                    {
                        std::string path = FileDialog::OpenFile({}).string();
                        if (!path.empty())
                        {
                            uiButtonComponent.pressedTexture = Texture2D::Load(path);
                        }
                    }
                }


                ImGui::Text("Base Size");
                ImGui::DragFloat2("##BaseSize", glm::value_ptr(uiButtonComponent.baseSize), 0.1f);


                ImGui::Text("Selected Size");
                ImGui::DragFloat2("##SelectedSize", glm::value_ptr(uiButtonComponent.selectedSize), 0.1f);


                ImGui::Text("Pressed Size");
                ImGui::DragFloat2("##PressedSize", glm::value_ptr(uiButtonComponent.pressedSize), 0.1f);


                ImGui::Text("Base Color");
                ImGui::ColorEdit4("##BaseColor", glm::value_ptr(uiButtonComponent.baseColor));


                ImGui::Text("Selected Color");
                ImGui::ColorEdit4("##SelectedColor", glm::value_ptr(uiButtonComponent.selectedColor));


                ImGui::Text("Pressed Color");
                ImGui::ColorEdit4("##PressedColor", glm::value_ptr(uiButtonComponent.pressedColor));

            }
        }

        if (entity.HasComponent<UIToggleComponent>())
        {
            auto& uiToggleComponent = entity.GetComponent<UIToggleComponent>();
            bool isCollapsingHeaderOpen = true;

            DrawAnchorPointCombo(uiToggleComponent.Anchor);

            if (ImGui::CollapsingHeader("UI Toggle", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text("Active Texture");
                if (uiToggleComponent.ActiveTexture)
                {
                    ImGui::Image((void*)(intptr_t)uiToggleComponent.ActiveTexture->GetID(), ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("No Active Texture Selected");
                }

                ImGui::Text("Inactive Texture");
                if (uiToggleComponent.InactiveTexture)
                {
                    ImGui::Image((void*)(intptr_t)uiToggleComponent.InactiveTexture->GetID(), ImVec2(64, 64));
                }
                else
                {
                    ImGui::Text("No Inactive Texture Selected");
                }

                ImGui::Checkbox("Is Active", &uiToggleComponent.IsActive);

                ImGui::Text("Size");
                ImGui::DragFloat2("##Size", glm::value_ptr(uiToggleComponent.Size), 0.1f);

                ImGui::Checkbox("Visible", &uiToggleComponent.Visible);

                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<UIToggleComponent>();
                }
            }
        }

        if (entity.HasComponent<AnimatorComponent>())
        {
            auto& animatorComponent = entity.GetComponent<AnimatorComponent>();

            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Animator", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                const char* animationName = animatorComponent.GetAnimationController()->GetAnimation(animatorComponent.CurrentAnimation)->GetAnimationName().c_str();

                if (ImGui::BeginCombo("Animation", animationName))
                {
                    for (auto& [name, animation] : animatorComponent.GetAnimationController()->GetAnimationMap())
                    {
                        if (ImGui::Selectable(name.c_str()) && name != animationName)
                        {
                            AnimationSystem::SetCurrentAnimation(name, &animatorComponent);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::DragFloat("Blend Duration", &animatorComponent.BlendDuration, 0.01f, 0.01f, 2.0f, "%.2f");

                ImGui::DragFloat("Blend Threshold", &animatorComponent.BlendThreshold, 0.01f, 0.01f, 1.0f, "%.2f");

                ImGui::DragFloat("Animation Speed", &animatorComponent.AnimationSpeed, 0.01f, 0.1f, 5.0f, "%.2f");

                ImGui::Checkbox("Loop", &animatorComponent.Loop);
            }

            if (!isCollapsingHeaderOpen)
            {
                // entity.RemoveComponent<AnimatorComponent>();
                // TODO remove animator component from entity and all the animation data
            }
        }
        if (entity.HasComponent<UICanvasComponent>())
        {
            auto& uiCanvasComponent = entity.GetComponent<UICanvasComponent>();
            bool isCollapsingHeaderOpen = true;

            // Display the UICanvas header in the inspector
            if (ImGui::CollapsingHeader("UI Canvas", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Remove the component if the header is closed
                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<UICanvasComponent>();
                }
            }
        }

        if(entity.HasComponent<ScriptComponent>())
        {
            auto& scriptComponent = entity.GetComponent<ScriptComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Script", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                // ImGui::Text("Script Path: ");
                // ImGui::Text(scriptComponent.script->GetPath().c_str());

                // Get the exposed variables
                auto& exposedVariables = scriptComponent.script->GetExportedVariables();

                // print the exposed variables
                for (auto& [name, variable] : exposedVariables)
                {
                    switch (variable.type)
                    {
                    case ExportedVariableType::Bool: {
                        bool value = variable.value.has_value() ? std::any_cast<bool>(variable.value) : false;
                        if (ImGui::Checkbox(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Int: {
                        int value = variable.value.has_value() ? std::any_cast<int>(variable.value) : 0;
                        if (ImGui::InputInt(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Float: {
                        float value = variable.value.has_value() ? std::any_cast<float>(variable.value) : 0.0f;
                        if (ImGui::InputFloat(name.c_str(), &value))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::String: {
                        std::string value =
                            variable.value.has_value() ? std::any_cast<std::string>(variable.value) : "";
                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, value.c_str());
                        if (ImGui::InputText(name.c_str(), buffer, sizeof(buffer)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)
                                ->SetVariable(name, std::string(buffer));
                            variable.value = std::string(buffer);
                        }
                        break;
                    }
                    case ExportedVariableType::Entity: {
                        Entity value = variable.value.has_value() ? std::any_cast<Entity>(variable.value) : Entity{};
                        if (ImGui::Button(name.c_str()))
                        {
                            ImGui::OpenPopup("EntityPopup");
                        }
                        if (ImGui::BeginPopup("EntityPopup"))
                        {
                            auto view = m_Context->m_Registry.view<TagComponent>();
                            for (auto entityID : view)
                            {
                                Entity e{entityID, m_Context.get()};
                                auto& tag = e.GetComponent<TagComponent>().Tag;
                                if (ImGui::Selectable(tag.c_str()))
                                {
                                    value = e;
                                    std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)
                                        ->SetVariable(name, value);
                                    variable.value = value;
                                }
                            }
                            ImGui::EndPopup();
                        }
                        break;
                    }
                    case ExportedVariableType::Vector2: {
                        glm::vec2 value =
                            variable.value.has_value() ? std::any_cast<glm::vec2>(variable.value) : glm::vec2{};
                        if (ImGui::DragFloat2(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Vector3: {
                        glm::vec3 value =
                            variable.value.has_value() ? std::any_cast<glm::vec3>(variable.value) : glm::vec3{};
                        if (ImGui::DragFloat3(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    case ExportedVariableType::Vector4: {
                        glm::vec4 value =
                            variable.value.has_value() ? std::any_cast<glm::vec4>(variable.value) : glm::vec4{};
                        if (ImGui::DragFloat4(name.c_str(), glm::value_ptr(value)))
                        {
                            std::dynamic_pointer_cast<LuaScript>(scriptComponent.script)->SetVariable(name, value);
                            variable.value = value;
                        }
                        break;
                    }
                    }
                }
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<ScriptComponent>();
            }
        }

        if (entity.HasComponent<NavMeshComponent>())
        {
            auto& navMeshComponent = entity.GetComponent<NavMeshComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("NavMesh", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Show NavMesh", &navMeshComponent.ShowDebug);
                ImGui::DragFloat("Walkable Slope Angle", &navMeshComponent.GetNavMesh()->WalkableSlopeAngle, 0.1f, 0.1f, 60.0f);

                if (ImGui::SmallButton("Generate NavMesh"))
                {
                    navMeshComponent.GetNavMesh()->CalculateWalkableAreas(entity.GetComponent<MeshComponent>().GetMesh(), entity.GetComponent<TransformComponent>().GetWorldTransform());
                }
            }
        }

        if (entity.HasComponent<NavigationAgentComponent>())
        {
            auto& navigationAgentComponent = entity.GetComponent<NavigationAgentComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Navigation Agent", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto view = m_Context->m_Registry.view<NavMeshComponent>();

                ImGui::Checkbox("Show Path", &navigationAgentComponent.ShowDebug);

                if (ImGui::BeginCombo("NavMesh", navigationAgentComponent.GetNavMeshComponent() ? std::to_string(navigationAgentComponent.GetNavMeshComponent()->GetNavMeshUUID()).c_str() : "Select NavMesh"))
                {
                    for (auto entityID : view)
                    {
                        Entity e{entityID, m_Context.get()};
                        auto& navMeshComponent = e.GetComponent<NavMeshComponent>();
                        bool isSelected = (navigationAgentComponent.GetNavMeshComponent() && navigationAgentComponent.GetNavMeshComponent()->GetNavMeshUUID() == navMeshComponent.GetNavMeshUUID());
                        if (ImGui::Selectable(std::to_string(navMeshComponent.GetNavMeshUUID()).c_str(), isSelected))
                        {
                            navigationAgentComponent.SetNavMeshComponent(CreateRef<NavMeshComponent>(navMeshComponent));
                            navigationAgentComponent.GetPathFinder()->SetNavMesh(navMeshComponent.GetNavMesh());
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
            }
        }

        if (entity.HasComponent<ParticlesSystemComponent>())
        {
            auto& particles = entity.GetComponent<ParticlesSystemComponent>();
            Ref<ParticleEmitter> emitter = particles.GetParticleEmitter();
            bool isCollapsingHeaderOpen = true;

            ImGui::PushID("ParticlesSystem");
            if (ImGui::CollapsingHeader("Particle System", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Position
                // ImGui::Text("Position");
                // ImGui::DragFloat3("##ParticlePosition", glm::value_ptr(particles.Position), 0.1f);

                // Rate over time

                // Direction
                ImGui::Checkbox("##ParticleDirectionUseRandom", &emitter->useDirectionRandom);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Direction");
                ImGui::DragFloat3("##ParticleDirectionNormal", glm::value_ptr(emitter->direction), 0.1f, -1.0f, 1.0f);
                if (emitter->useDirectionRandom)
                {
                    ImGui::DragFloat3("##ParticleDirectionRandom", glm::value_ptr(emitter->directionRandom), 0.1f,
                                      -1.0f, 1.0f);
                }

                // Colour
                ImGui::Checkbox("##ParticleColorUseRandom", &emitter->useColorRandom);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Colour");
                ImGui::ColorEdit4("##ParticleColourNormal", glm::value_ptr(emitter->colorNormal));
                if (emitter->useColorRandom)
                {
                    ImGui::ColorEdit4("##ParticleColorRandom", glm::value_ptr(emitter->colorRandom));
                }

                //// Life Time
                // ImGui::Text("Life Time");
                // ImGui::DragFloat("##ParticleLife", &emitter->lifeTime, 0.1f, 0.0f, 100.0f);

                //// Size
                // ImGui::Text("Size");
                // ImGui::DragFloat("##ParticleSize", &emitter->size, 0.1f, 0.0f, 10.0f);

                // Looping
                ImGui::Checkbox("##ParticleLooping", &emitter->looping);
                ImGui::SameLine();
                ImGui::Text("Looping");

                // Start Life Time
                // Use Random Start Life Time
                ImGui::Checkbox("##UseRandomStartLifeTime", &emitter->useRandomLifeTime);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Start Life Time");
                if (emitter->useRandomLifeTime)
                {
                    ImGui::Text("Min");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleStartLifeTimeMin", &emitter->startLifeTimeMin, 0.1f, 0.0f, 100.0f);
                    ImGui::Text("Max");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleStartLifeTimeMax", &emitter->startLifeTimeMax, 0.1f, 0.0f, 100.0f);
                }
                else
                {
                    ImGui::DragFloat("##ParticleStartLifeTime", &emitter->startLifeTime, 0.1f, 0.0f, 100.0f);
                }
                ImGui::Separator();

                // Start speed
                // Use Random Start sppeed
                ImGui::Checkbox("##UseRandomStartSpeed", &emitter->useRandomSpeed);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Start Speed");

                if (emitter->useRandomSpeed)
                {
                    ImGui::Text("Min");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleStartSpeedMin", &emitter->startSpeedMin, 0.1f, 0.0f, 100.0f);
                    ImGui::Text("Max");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleStartSpeedMax", &emitter->startSpeedMax, 0.1f, 0.0f, 100.0f);
                }
                else
                {
                    ImGui::DragFloat("##ParticleStartSpeed", &emitter->startSpeed, 0.1f, 0.0f, 100.0f);
                }

                // Start Size
                // Use Random Start Size
                ImGui::Checkbox("##UseRandomStartSize", &emitter->useRandomSize);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Start Size");
                ImGui::SameLine();
                ImGui::Checkbox("##UseSplitAxesStartSize", &emitter->useSplitAxesSize);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to split the startSize value in to three axes");
                }

                if (emitter->useSplitAxesSize)
                {

                    if (emitter->useRandomSize)
                    {

                        ImGui::Text("Min");
                        ImGui::SameLine();
                        ImGui::DragFloat3("##UseAxesParticleStartSizeMin", glm::value_ptr(emitter->startSizeMin), 0.1f);
                        ImGui::Text("Max");
                        ImGui::SameLine();
                        ImGui::DragFloat3("##UseAxesParticleStartSizeMax", glm::value_ptr(emitter->startSizeMax), 0.1f);
                    }
                    else
                    {
                        ImGui::DragFloat3("##UseAxesParticleStartSize", glm::value_ptr(emitter->startSize), 0.1f);
                    }
                }
                else
                {

                    if (emitter->useRandomSize)
                    {
                        float unirformValueSizeMin = emitter->startSizeMin.x;
                        float unirformValueSizeMax = emitter->startSizeMax.x;

                        ImGui::Text("Min");
                        ImGui::SameLine();
                        if (ImGui::DragFloat("##NoAxesParticleStartSizeMin", &unirformValueSizeMin, 0.1f))
                        {
                            emitter->startSizeMin = glm::vec3(unirformValueSizeMin);
                        }
                        ImGui::Text("Max");
                        ImGui::SameLine();
                        if (ImGui::DragFloat("##NoAxesParticleStartSizeMax", &unirformValueSizeMax, 0.1f))
                        {
                            emitter->startSizeMax = glm::vec3(unirformValueSizeMax);
                        }
                    }
                    else
                    {
                        float unirformValueSize = emitter->startSize.x;
                        if (ImGui::DragFloat("##NoAxesParticleStartSize", &unirformValueSize, 0.1f))
                        {
                             emitter->startSize = glm::vec3(unirformValueSize);
                         }
                    }
                }

                // Start Rotation
                // Use Random Start Rotation
                ImGui::Checkbox("##UseRandomStartRotation", &emitter->useRandomRotation);
                if (ImGui::IsItemHovered())
                {
                    ImGui::SetTooltip("Check this button to use the random system.\nThe value above is the min, and "
                                      "the value below is the max.");
                }
                ImGui::SameLine();
                ImGui::Text("Start Rotation");

                if (emitter->useRandomRotation)
                {
                    ImGui::Text("Min");
                    ImGui::SameLine();
                    ImGui::DragFloat3("##ParticleStartRotationMin", glm::value_ptr(emitter->startRotationMin), 0.1f);
                    ImGui::Text("Max");
                    ImGui::SameLine();
                    ImGui::DragFloat3("##ParticleStartRotationMax", glm::value_ptr(emitter->startRotationMax), 0.1f);
                }
                else
                {
                    ImGui::DragFloat3("##ParticleStartRotation", glm::value_ptr(emitter->startRotation), 0.1f);
                }

                // Simulation Space
                // ImGui::Text("Simulation Space");
                // ImGui::SameLine();

                //// Show Combo Menu
                // const char* simulationSpaceOptions[] = {"Local", "World", "Custom"};
                // int currentSimulationSpace = static_cast<int>(emitter->simulationSpace);

                // if (ImGui::Combo("##SimulationSpace", &currentSimulationSpace, simulationSpaceOptions,
                //                  IM_ARRAYSIZE(simulationSpaceOptions)))
                //{
                //     emitter->simulationSpace = static_cast<ParticleEmitter::SimulationSpace>(currentSimulationSpace);
                // }

                ImGui::Checkbox("##UseEmission", &emitter->useEmission);
                ImGui::SameLine();
                ImGui::PushID("Emission");

                if (ImGui::TreeNodeEx("Emission Settings", ImGuiTreeNodeFlags_None))
                {
                    // If not enabled, set the text to gray and disable the controls
                    if (!emitter->useEmission)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                    }

                    // Select emitter shape
                    ImGui::Text("Rate over Time");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleRateOverTime", &emitter->rateOverTime, 0.1, 0);

                    ImGui::TreePop();
                }
                ImGui::PopID();

                // Shape section: Select shape and control other properties (Angle, Radius, Radius Thickness)
                ImGui::Checkbox("##UseShape", &emitter->useShape); // Shape toggle checkbox
                ImGui::SameLine();
                ImGui::PushID("Shape");

                if (ImGui::TreeNodeEx("Shape Settings", ImGuiTreeNodeFlags_None))
                {
                    // If not enabled, set the text to gray and disable the controls
                    if (!emitter->useShape)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                    }

                    // Select emitter shape
                    const char* shapeNames[] = {"Sphere", "Cone", "Box"};
                    ImGui::Text("Shape");
                    ImGui::SameLine();
                    ImGui::Combo("##ShapeType", reinterpret_cast<int*>(&emitter->shape), shapeNames,
                                 IM_ARRAYSIZE(shapeNames));

                    // Spread
                    ImGui::Text("Spread");
                    /*ImGui::SameLine();*/
                    ImGui::DragFloat3("##ParticleSpreadMin", glm::value_ptr(emitter->minSpread), 0.1f);
                    ImGui::DragFloat3("##ParticleSpreadMax", glm::value_ptr(emitter->maxSpread), 0.1f);

                    // Control the angle (only applies to Cone)
                    if (emitter->shape == ParticleEmitter::ShapeType::Cone)
                    {
                        ImGui::Text("Angle");
                        ImGui::SameLine();
                        ImGui::DragFloat("##Angle", &emitter->shapeAngle, 1.0f, 0.0f,
                                         180.0f); // Control angle, range: 0 to 180
                    }

                    if (emitter->shape != ParticleEmitter::ShapeType::Box)
                    {

                        // Control the radius
                        ImGui::Text("Radius");
                        ImGui::SameLine();
                        ImGui::DragFloat("##Radius", &emitter->shapeRadius, 0.1f, 0.0f,
                                         100.0f); // Control radius, range: 0 to 100

                        // Control radius thickness (for ring-shaped emitter)
                        ImGui::Text("Radius Thickness");
                        ImGui::SameLine();
                        ImGui::DragFloat("##RadiusThickness", &emitter->shapeRadiusThickness, 0.01f, 0.0f,
                                         10.0f); // Range: 0 to 10
                    }

                    // Restore the default state
                    if (!emitter->useShape)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleColor();
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();

                // Velocity Over Lifetime - Checkbox and Collapsing Header
                ImGui::Checkbox("##UseVelocityOverLifetime", &emitter->useVelocityOverLifetime);

                // SameLine to make checkbox and header appear on the same line
                ImGui::SameLine();
                ImGui::PushID("VelocityOverLifetime");

                // Use TreeNodeEx to create a collapsible panel without a close button
                if (ImGui::TreeNodeEx("Velocity Over Lifetime Settings", ImGuiTreeNodeFlags_None))
                {
                    // If the checkbox is not selected, set controls to gray and disable them
                    if (!emitter->useVelocityOverLifetime)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out text
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                    }

                    ImGui::Checkbox("Separate Axes", &emitter->velocityOverLifeTimeSeparateAxes);

                    if (emitter->velocityOverLifeTimeSeparateAxes)
                    {

                        ImGui::Text("Velocity X");
                        CurveEditor::DrawCurve("Velocity X", emitter->speedOverLifeTimeX);

                        ImGui::Text("Velocity Y ");
                        CurveEditor::DrawCurve("Velocity Y", emitter->speedOverLifeTimeY);

                        ImGui::Text("Velocity Z");
                        CurveEditor::DrawCurve("Velocity Z", emitter->speedOverLifeTimeZ);
                    }
                    else
                    {

                        ImGui::Text("Velocity");
                        CurveEditor::DrawCurve("Velocity", emitter->speedOverLifeTimeGeneral);
                    }

                    // Restore default state
                    if (!emitter->useVelocityOverLifetime)
                    {
                        ImGui::PopItemFlag();   // Restore control state
                        ImGui::PopStyleColor(); // Restore color
                    }

                    // Close tree node
                    ImGui::TreePop();
                }

                ImGui::PopID();

                // Color Over Lifetime - Checkbox and Collapsing Header
                ImGui::Checkbox("##UseColorOverLifetime", &emitter->useColorOverLifetime);

                // SameLine to make checkbox and header appear on the same line
                ImGui::SameLine();
                ImGui::PushID("ColorOverLifetime");

                // Use TreeNodeEx to create a collapsible panel without a close button
                if (ImGui::TreeNodeEx("Color Over Lifetime Settings", ImGuiTreeNodeFlags_None))
                {
                    // If the checkbox is not selected, set controls to gray and disable them
                    if (!emitter->useColorOverLifetime)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out text
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                    }

                    // Gradient option (Placeholder: Need to implement gradient system)
                    ImGui::Text("Gradient");
                    ImGui::SameLine();
                    // if (ImGui::Button("Edit Gradient"))
                    //{
                    //     // Open a gradient editor (Needs implementation)
                    // }

                    GradientEditor::ShowGradientEditor(emitter->colorOverLifetime_gradientPoints);

                    // Restore default state
                    if (!emitter->useColorOverLifetime)
                    {
                        ImGui::PopItemFlag();   // Restore control state
                        ImGui::PopStyleColor(); // Restore color
                    }

                    // Close tree node
                    ImGui::TreePop();
                }

                ImGui::PopID();

                // Size Over Lifetime - Checkbox and Collapsing Header
                ImGui::Checkbox("##UseSizeOverLifetime", &emitter->useSizeOverLifetime);

                ImGui::SameLine();
                ImGui::PushID("SizeOverLifetime");

                if (ImGui::TreeNodeEx("Size Over Lifetime Settings", ImGuiTreeNodeFlags_None))
                {
                    // If not enabled, set text to gray and disable controls
                    if (!emitter->useSizeOverLifetime)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    }

                    // Enable or disable separate XYZ axes
                    ImGui::Checkbox("Separate Axes", &emitter->sizeOverLifeTimeSeparateAxes);

                    if (emitter->sizeOverLifeTimeSeparateAxes)
                    {
                        ImGui::Text("Size X");
                        CurveEditor::DrawCurve("Size X", emitter->sizeOverLifetimeX);

                        ImGui::Text("Size Y");
                        CurveEditor::DrawCurve("Size Y", emitter->sizeOverLifetimeY);

                        ImGui::Text("Size Z");
                        CurveEditor::DrawCurve("Size Z", emitter->sizeOverLifetimeZ);
                    }
                    else
                    {
                        ImGui::Text("Size");
                        CurveEditor::DrawCurve("Size", emitter->sizeOverLifetimeGeneral);
                    }

                    // Restore default state
                    if (!emitter->useSizeOverLifetime)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleColor();
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();

                // Rotation Over Lifetime - Checkbox and Collapsing Header
                ImGui::Checkbox("##UseRotationOverLifetime", &emitter->useRotationOverLifetime);

                ImGui::SameLine();
                ImGui::PushID("RotationOverLifetime");

                if (ImGui::TreeNodeEx("Rotation Over Lifetime Settings", ImGuiTreeNodeFlags_None))
                {
                    // If not enabled, set text to gray and disable controls
                    if (!emitter->useRotationOverLifetime)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    }

                    // Rotation on X axis
                    ImGui::Text("Rotation X");
                    CurveEditor::DrawCurve("##RotationX", emitter->rotationOverLifetimeX);

                    // Rotation on Y axis
                    ImGui::Text("Rotation Y");
                    CurveEditor::DrawCurve("##RotationY", emitter->rotationOverLifetimeZ);

                    // Rotation on Z axis
                    ImGui::Text("Rotation Z");
                    CurveEditor::DrawCurve("##RotationZ", emitter->rotationOverLifetimeY);

                    // Restore default state
                    if (!emitter->useRotationOverLifetime)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleColor();
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();

                // Renderer - Checkbox and Collapsing Header
                ImGui::Checkbox("##UseRenderer", &emitter->useRenderer);

                ImGui::SameLine();
                ImGui::PushID("Renderer");

                if (ImGui::TreeNodeEx("Renderer Settings", ImGuiTreeNodeFlags_None))
                {
                    // If not enabled, set text to gray and disable controls
                    if (!emitter->useRenderer)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray out
                        ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                    }

                    // Particle Amount
                    ImGui::Text("Max Particles");
                    ImGui::SameLine();
                    ImGui::DragInt("##ParticleAmount", &emitter->amount, 1, 1, 10000);

                    // Render Mode selection
                    const char* renderModes[] = {"Billboard", "Custom"};
                    ImGui::Text("Render Alignment");
                    ImGui::SameLine();
                    ImGui::Combo("##RenderAlignment", reinterpret_cast<int*>(&emitter->renderAlignment), renderModes,
                                 IM_ARRAYSIZE(renderModes));

                    //ImGui::SameLine();
                    //if (ImGui::Button("Select Material"))
                    //{
                    //    // Open Material selection logic here
                    //}

                    //// Texture Selector
                    // ImGui::Text("Texture");
                    // ImGui::SameLine();
                    // if (ImGui::Button("Select Texture"))
                    //{
                    //     // Open texture selection logic here
                    // }

                    // Render Alignment selection
                    /*const char* renderAlignments[] = {"View", "Local", "World"};
                    ImGui::Text("Render Alignment");
                    ImGui::SameLine();
                    ImGui::Combo("##RenderAlignment", reinterpret_cast<int*>(&emitter->renderAlignment),
                                 renderAlignments, IM_ARRAYSIZE(renderAlignments));*/

                    // Restore default state
                    if (!emitter->useRenderer)
                    {
                        ImGui::PopItemFlag();
                        ImGui::PopStyleColor();
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<ParticlesSystemComponent>();
            }
            ImGui::PopID();
        }

        ImGui::Separator();

        ImGui::Dummy(ImVec2(0.0f, 10.0f));

        float buttonWidth = 200.0f;
        float buttonHeight = 32.0f;
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float cursorPosX = (availableWidth - buttonWidth) * 0.5f;
        ImGui::SetCursorPosX(cursorPosX);

        if (ImGui::Button("Add Component", {buttonWidth, buttonHeight}))
        {
            ImGui::OpenPopup("Add Component...");
        }

        if (ImGui::BeginPopupModal("Add Component..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:", buffer, 256);

            std::string items[] = { "Tag Component", "Transform Component", "Mesh Component", "Material Component", "Light Component", "Camera Component", "Audio Source Component", "Audio Listener Component", "Audio Zone Component", "Lua Script Component", "Rigidbody Component", "NavMesh Component", "Navigation Agent Component", "UI Canvas Component", "Image Component", "Text Component", "Slider Component", "Button Component", "Toggle Component", "Particles System Component" };

            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer "
                               "scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi "
                               "lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Component"))
            {
                if (items[item_current] == "Tag Component")
                {
                    if (!entity.HasComponent<TagComponent>())
                        entity.AddComponent<TagComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Transform Component")
                {
                    if (!entity.HasComponent<TransformComponent>())
                        entity.AddComponent<TransformComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Mesh Component")
                {
                    if (!entity.HasComponent<MeshComponent>())
                        entity.AddComponent<MeshComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Material Component")
                {
                    if(!entity.HasComponent<MaterialComponent>())
                    {
                        entity.AddComponent<MaterialComponent>(Material::Create("Default Material"));
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Light Component")
                {
                    if (!entity.HasComponent<LightComponent>())
                        entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Camera Component")
                {
                    if (!entity.HasComponent<CameraComponent>())
                        entity.AddComponent<CameraComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Audio Source Component")
                {
                    if (!entity.HasComponent<AudioSourceComponent>())
                    {
                        entity.AddComponent<AudioSourceComponent>();
                        Audio::RegisterAudioSourceComponent(entity.GetComponent<AudioSourceComponent>());
                        AudioZone::RegisterObject(entity.GetComponent<AudioSourceComponent>().gameObjectID,
                                                  entity.GetComponent<AudioSourceComponent>().transform[3]);
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Audio Listener Component")
                {
                    if (!entity.HasComponent<AudioListenerComponent>())
                    {
                        entity.AddComponent<AudioListenerComponent>();
                        Audio::RegisterAudioListenerComponent(entity.GetComponent<AudioListenerComponent>());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Audio Zone Component")
                {
                    if (!entity.HasComponent<AudioZoneComponent>())
                    {
                        entity.AddComponent<AudioZoneComponent>();
                        AudioZone::CreateZone(entity.GetComponent<AudioZoneComponent>());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Lua Script Component")
                {
                    if (!entity.HasComponent<ScriptComponent>())
                    {
                        m_ShowLuaScriptOptions = true;
                        ImGui::CloseCurrentPopup();
                    }
                    else
                    {
                        ImGui::CloseCurrentPopup();
                    }
                }
                else if (items[item_current] == "Particles System Component")
                {
                    if (!entity.HasComponent<ParticlesSystemComponent>())
                    {

                        entity.AddComponent<ParticlesSystemComponent>();
                        if (!entity.HasComponent<MaterialComponent>())
                         {
                             entity.AddComponent<MaterialComponent>(Material::Create("Default Particle Material"));
                             
                         }
                        ImGui::CloseCurrentPopup();
                    }
                }  
                else if(items[item_current] == "Rigidbody Component")
                {
                    if(!entity.HasComponent<RigidbodyComponent>())
                    {
                        try {
                            Ref<BoxCollider> collider = CreateRef<BoxCollider>(glm::vec3(1.0f, 1.0f, 1.0f));

                            RigidBody::Properties props;
                            props.type = RigidBody::Type::Dynamic;
                            props.mass = 1.0f;
                            props.useGravity = true;

                            auto& rbComponent = entity.AddComponent<RigidbodyComponent>(props, collider);

                            // Set initial transform from the entity
                            if (entity.HasComponent<TransformComponent>()) {
                                auto& transform = entity.GetComponent<TransformComponent>();
                                rbComponent.rb->SetPosition(transform.Position);
                                rbComponent.rb->SetRotation(transform.Rotation);
                            }

                            // Add the rigidbody to the physics world
                            m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());

                            // Set user pointer for collision detection
                            rbComponent.rb->GetNativeBody()->setUserPointer(
                                reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                        }
                        catch (const std::exception& e) {
                            COFFEE_CORE_ERROR("Exception creating rigidbody: {0}", e.what());
                            if (entity.HasComponent<RigidbodyComponent>()) {
                                entity.RemoveComponent<RigidbodyComponent>();
                            }
                        }
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "NavMesh Component")
                {
                    if(!entity.HasComponent<NavMeshComponent>() && entity.HasComponent<MeshComponent>() && entity.HasComponent<TransformComponent>())
                    {
                        auto& navMeshComponent = entity.AddComponent<NavMeshComponent>();
                        navMeshComponent.SetNavMesh(CreateRef<NavMesh>());
                        navMeshComponent.SetNavMeshUUID(UUID());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if(items[item_current] == "Navigation Agent Component")
                {
                    if(!entity.HasComponent<NavigationAgentComponent>())
                    {
                        auto& navigationAgentComponent = entity.AddComponent<NavigationAgentComponent>();
                        navigationAgentComponent.SetPathFinder(CreateRef<NavMeshPathfinding>(nullptr));
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Canvas Component")
                {
                    if (!entity.HasComponent<UICanvasComponent>())
                        entity.AddComponent<UICanvasComponent>();
                    auto& uiCanvasComponent = entity.GetComponent<UICanvasComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Image Component")
                {
                    if (!entity.HasComponent<UIImageComponent>())
                        entity.AddComponent<UIImageComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Text Component")
                {
                    if (!entity.HasComponent<UITextComponent>())
                        entity.AddComponent<UITextComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Slider Component")
                {
                    if (!entity.HasComponent<UISliderComponent>())
                        entity.AddComponent<UISliderComponent>();
                    ImGui::CloseCurrentPopup();
                }
                if (items[item_current] == "Button Component")
                {
                    if (!entity.HasComponent<UIButtonComponent>())
                    {
                        entity.AddComponent<UIButtonComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                if (items[item_current] == "Toggle Component")
                {
                    if (!entity.HasComponent<UIToggleComponent>())
                    {
                        entity.AddComponent<UIToggleComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

        // Add Lua script component options
        if (m_ShowLuaScriptOptions)
        {
            ImGui::OpenPopup("Lua Script Source");
            m_ShowLuaScriptOptions = false;
        }

        // Make sure your Lua Script Source popup is handled outside any other popup context
        if (ImGui::BeginPopupModal("Lua Script Source", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Select script source:");
            ImGui::Separator();

            if (ImGui::Button("Create New Script", ImVec2(200, 0)))
            {
                // Pop up a file dialog to select the save location for the new script
                FileDialogArgs args;
                args.Filters = {{"Lua Script", "lua"}};
                args.DefaultName = "NewScript.lua";
                const std::filesystem::path& path = FileDialog::SaveFile(args);

                if (!path.empty())
                {
                    std::ofstream scriptFile(path);
                    if (scriptFile.is_open())
                    {
                        scriptFile << "function on_ready()\n";
                        scriptFile << "    -- Add initialization code here\n";
                        scriptFile << "end\n\n";
                        scriptFile << "function on_update(dt)\n";
                        scriptFile << "    -- Add update code here\n";
                        scriptFile << "end\n\n";
                        scriptFile << "function on_exit()\n";
                        scriptFile << "    -- Add cleanup code here\n";
                        scriptFile << "end\n";
                        scriptFile.close();

                        // Add the script component to the entity
                        entity.AddComponent<ScriptComponent>(path.string(), ScriptingLanguage::Lua);
                    }
                    else
                    {
                        COFFEE_CORE_ERROR("Failed to create Lua script file at: {0}", path.string());
                    }
                }

                ImGui::CloseCurrentPopup();
            }

            if (ImGui::Button("Open Existing Script", ImVec2(200, 0)))
            {
                FileDialogArgs args;
                args.Filters = {{"Lua Script", "lua"}};
                const std::filesystem::path& path = FileDialog::OpenFile(args);

                if (!path.empty())
                {
                    // Add the script component to the entity with the selected script
                    entity.AddComponent<ScriptComponent>(path.string(), ScriptingLanguage::Lua);
                }

                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            if (ImGui::Button("Cancel", ImVec2(200, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    // UI functions for scenetree menus
    void SceneTreePanel::ShowCreateEntityMenu()
    {
        if (ImGui::BeginPopupModal("Add Entity..."))
        {
            static char buffer[256] = "";
            ImGui::InputTextWithHint("##Search Component", "Search Component:", buffer, 256);

            std::string items[] = {"Empty", "Camera", "Primitive", "Light", "UI Canvas", "Particle System"};

            static int item_current = 1;

            if (ImGui::BeginListBox("##listbox 2", ImVec2(-FLT_MIN, ImGui::GetContentRegionAvail().y - 200)))
            {
                for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                {
                    const bool is_selected = (item_current == n);
                    if (ImGui::Selectable(items[n].c_str(), is_selected))
                        item_current = n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }

            ImGui::Text("Description");
            ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Cras vel odio lectus. Integer "
                               "scelerisque lacus a elit consequat, at imperdiet felis feugiat. Nunc rhoncus nisi "
                               "lacinia elit ornare, eu semper risus consectetur.");

            if (ImGui::Button("Cancel"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("Add Component"))
            {
                if (items[item_current] == "Empty")
                {
                    Entity e = m_Context->CreateEntity();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Camera")
                {
                    Entity e = m_Context->CreateEntity("Camera");
                    e.AddComponent<CameraComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Primitive")
                {
                    Entity e = m_Context->CreateEntity("Primitive");
                    e.AddComponent<MeshComponent>();
                    e.AddComponent<MaterialComponent>(Material::Create("Default Material"));
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Light")
                {
                    Entity e = m_Context->CreateEntity("Light");
                    e.AddComponent<LightComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Canvas")
                {
                    Entity e = m_Context->CreateEntity("UI Canvas");
                    e.AddComponent<UICanvasComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Particle System")
                {
                    Entity e = m_Context->CreateEntity("ParticleSystem");
                    e.AddComponent<ParticlesSystemComponent>();
                    e.AddComponent<MaterialComponent>(Material::Create("Default Particle Material"));
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }
    }
} // namespace Coffee