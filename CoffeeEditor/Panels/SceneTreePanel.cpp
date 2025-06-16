#include "SceneTreePanel.h"

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Core/FileDialog.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/Renderer/Camera.h"
#include "CoffeeEngine/Renderer/Material.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Renderer3D.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/Prefab.h"
#include "CoffeeEngine/Scene/PrimitiveMesh.h"
#include "CoffeeEngine/Scene/Scene.h"
#include "CoffeeEngine/Scene/SceneCamera.h"
#include "CoffeeEngine/Scene/SceneTree.h"
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include "CoffeeEngine/UI/UIAnchor.h"
#include "CoffeeEngine/UI/UIManager.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "imgui_internal.h"
#include <IconsLucide.h>

#include <CoffeeEngine/Scripting/Script.h>
#include <SDL3/SDL_misc.h>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
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

        // Search by entity tag
        static std::array<char, 256> searchBuffer;
        ImGui::InputTextWithHint("##searchbar", ICON_LC_SEARCH " Search by name:", searchBuffer.data(),
                                 searchBuffer.size());

        ImGui::SameLine();
        if (ImGui::Button(ICON_LC_CIRCLE_X "##searchbarclear"))
        {
            searchBuffer[0] = '\0';
        }

        ImGui::BeginChild("entity tree", {0, 0}, ImGuiChildFlags_Border);

        bool searchMode = (searchBuffer[0] != '\0');
        std::string search = searchBuffer.data();
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);

        auto view = m_Context->m_Registry.view<entt::entity>();
        for (auto entityID : view)
        {
            Entity entity{entityID, m_Context.get()};

            if (searchMode)
            {
                // Find substring in tag, draw entity if found and continue to next entity
                auto tag = entity.GetComponent<TagComponent>().Tag;
                std::transform(tag.begin(), tag.end(), tag.begin(), ::tolower);
                if (tag.find(search) != std::string::npos)
                {

                    DrawEntityNode(entity, false);
                }
                continue;
            }

            auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

            if (hierarchyComponent.m_Parent == entt::null)
            {
                DrawEntityNode(entity);
            }
        }

        ImGui::EndChild();

        // Entity unparenting
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
            {
                // Unparent the entity if dragged onto empty space in the hierarchy view
                // Copy-paste of payload handling for reparenting entities found in DrawEntityNode(entity)
                Entity payloadEntity = *(const Entity*)payload->Data;
                HierarchyComponent::Reparent(
                    m_Context->m_Registry, (entt::entity)payloadEntity,
                    entt::null); // Parent set to null (unparented)
            }
            ImGui::EndDragDropTarget();
        }

        // Entity Tree Drag and Drop functionality
        if (ImGui::BeginDragDropTarget())
        {
            // Handle normal resources (like models)
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
                case ResourceType::Prefab: {
                    if (const Ref<Prefab> prefab = std::static_pointer_cast<Prefab>(resource))
                    {
                        const Entity instance = prefab->Instantiate(m_Context.get());
                        SetSelectedEntity(instance);

                        COFFEE_CORE_INFO("Instantiated prefab: {0}", prefab->GetPath().string());
                    }
                    break;
                }
                default: {
                    break;
                }
                } // End of switch
            }

            // Handle prefab paths - only load the prefab when it's actually dropped
            if (const ImGuiPayload* prefabPayload = ImGui::AcceptDragDropPayload("PREFAB_PATH"))
            {
                const char* pathStr = (const char*)prefabPayload->Data;
                std::filesystem::path prefabPath = pathStr;

                // Load the prefab now that it's being used
                Ref<Prefab> prefab = Prefab::Load(prefabPath);
                if (prefab)
                {
                    Entity instance = prefab->Instantiate(m_Context.get());
                    SetSelectedEntity(instance);

                    COFFEE_CORE_INFO("Instantiated prefab: {0}", prefabPath.string());
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

    void SceneTreePanel::DrawEntityNode(Entity entity, bool drawChildren)
    {
        auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;
        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

        ImGuiTreeNodeFlags flags = ((m_SelectionContext == entity) ? ImGuiTreeNodeFlags_Selected : 0) |
                                   ((!drawChildren || hierarchyComponent.m_First == entt::null) ? ImGuiTreeNodeFlags_Leaf : 0) |
                                   ((drawChildren) ? ImGuiTreeNodeFlags_OpenOnArrow : 0) | ImGuiTreeNodeFlags_FramePadding;

        bool isActive = entity.IsActive();
        const char* icon = isActive ? ICON_LC_EYE : ICON_LC_EYE_OFF;
        std::string buttonId = "##Active" + std::to_string((uint32_t)entity);

        if (ImGui::GetDragDropPayload()
            && ImGui::GetMousePos().y > ImGui::GetCursorScreenPos().y
            && ImGui::GetMousePos().y < ImGui::GetCursorScreenPos().y + 6)
        {
            ImGui::InvisibleButton("##DropTarget", ImVec2(ImGui::GetContentRegionAvail().x, 6));
        }

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY_NODE"))
            {
                // Assuming payload is an Entity, but you need to cast and check appropriately
                Entity payloadEntity = *(const Entity*)payload->Data;
                // Process the drop, e.g., reordering the entity in the hierarchy
                // This is where you would update the ECS or scene graph
                HierarchyComponent::Reorder(m_Context->m_Registry,payloadEntity, entt::null, entity);
            }
            ImGui::EndDragDropTarget();
        }

        ImGui::SetNextItemAllowOverlap();

        // Draw the tree node first, so ImGui sets up the proper indentation
        bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, entityNameTag.c_str());

        if (ImGui::IsItemClicked())
        {
            m_SelectionContext = entity;
        }

        // Create a unique popup ID for each entity to prevent collisions
        std::string contextMenuId = "EntityContextMenu##" + std::to_string((uint32_t)(uint64_t)(entt::entity)entity);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            // Set the selection context to this entity and open its unique popup
            m_SelectionContext = entity;
            ImGui::OpenPopup(contextMenuId.c_str());
        }

        if (ImGui::BeginPopup(contextMenuId.c_str()))
        {
            if (ImGui::MenuItem("Create Prefab"))
            {
                CreatePrefab(entity);
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
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

        // Calculate the eye icon position based on current indentation level
        // This fixes the issue where eye icon moves left when entity becomes a child
        float iconPosition = ImGui::GetWindowContentRegionMax().x -
                             ImGui::CalcTextSize(icon).x -
                             ImGui::GetStyle().FramePadding.x * 2.0f;

        // Set cursor position to align icon to the right
        float currentX = ImGui::GetCursorPosX();
        ImGui::SameLine();
        ImGui::SetCursorPosX(iconPosition);

        // Style the icon button
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 0.8f));

        // Create button with just the icon
        if (ImGui::Button((icon + buttonId).c_str()))
        {
            // Toggle active state when clicked
            isActive = !isActive;
            entity.SetActive(isActive);
        }

        // Restore original style
        ImGui::PopStyleColor(3);

        if (opened)
        {
            if (drawChildren && hierarchyComponent.m_First != entt::null)
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

    void SceneTreePanel::DrawTransform(TransformComponent& transformComponent)
    {
        glm::vec3 position = transformComponent.GetLocalPosition();
        glm::vec3 rotation = transformComponent.GetLocalRotation();
        glm::vec3 scale = transformComponent.GetLocalScale();

        ImGui::Text("Position");
        if (ImGui::DragFloat3("##Position", glm::value_ptr(position), 0.1f))
        {
            transformComponent.SetLocalPosition(position);
        }

        ImGui::Text("Rotation");
        if (ImGui::DragFloat3("##Rotation", glm::value_ptr(rotation), 0.1f))
        {
            transformComponent.SetLocalRotation(rotation);
        }

        ImGui::Text("Scale");
        if (ImGui::DragFloat3("##Scale", glm::value_ptr(scale), 0.1f))
        {
            transformComponent.SetLocalScale(scale);
        }
    }

    void SceneTreePanel::DrawUITransform(TransformComponent& transformComponent, RectAnchor& anchor, Entity entity)
    {
        if (ImGui::Button("Anchor Presets"))
            ImGui::OpenPopup("AnchorPresetsPopup");

        if (ImGui::BeginPopup("AnchorPresetsPopup"))
        {
            ImGui::Text("Alt: Also set position");
            ImGui::Separator();

            bool preservePosition = !ImGui::GetIO().KeyAlt;

            static const char* rowLabels[] = { "top", "middle", "bottom", "stretch" };
            static const char* columnLabels[] = { "left", "center", "right", "stretch" };

            ImGui::BeginTable("AnchorPresets", 5);

            ImGui::TableNextColumn();
            for (int i = 0; i < 4; i++)
            {
                ImGui::TableNextColumn();
                ImGui::Text("%s", columnLabels[i]);
            }

            for (int row = 0; row < 4; row++)
            {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s", rowLabels[row]);

                for (int col = 0; col < 4; col++)
                {
                    ImGui::TableNextColumn();

                    std::string buttonIdStr = "##anchor" + std::to_string(row) + std::to_string(col);

                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
                    if (ImGui::Button(buttonIdStr.c_str(), ImVec2(24, 24)))
                    {
                        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();

                        Entity parentEntity{hierarchyComponent.m_Parent, m_Context.get()};
                        auto& parentRenderItem = UIManager::GetUIRenderItem(parentEntity);
                        glm::vec2 parentSize = UIManager::GetParentSize(m_Context->m_Registry, parentRenderItem);

                        glm::vec4 currentRect = anchor.CalculateRect(parentSize);

                        AnchorPreset preset = UIManager::GetAnchorPreset(row, col);
                        anchor.SetAnchorPreset(preset, currentRect, parentSize, preservePosition);
                        UIManager::MarkDirty(entity);

                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopStyleVar();

                    ImVec2 buttonMin = ImGui::GetItemRectMin();
                    ImVec2 buttonMax = ImGui::GetItemRectMax();
                    ImVec2 buttonSize = ImVec2(buttonMax.x - buttonMin.x, buttonMax.y - buttonMin.y);
                    ImDrawList* drawList = ImGui::GetWindowDrawList();

                    drawList->AddRect(buttonMin, buttonMax, IM_COL32(255, 255, 255, 100));

                    float padding = 6.0f;
                    ImVec2 innerMin, innerMax;

                    switch (col) {
                        case 0: // Left
                            innerMin.x = buttonMin.x + padding;
                            innerMax.x = buttonMin.x + buttonSize.x / 2.0f;
                            break;
                        case 1: // Center
                            innerMin.x = buttonMin.x + buttonSize.x / 4.0f;
                            innerMax.x = buttonMax.x - buttonSize.x / 4.0f;
                            break;
                        case 2: // Right
                            innerMin.x = buttonMin.x + buttonSize.x / 2.0f;
                            innerMax.x = buttonMax.x - padding;
                            break;
                        case 3: // Stretch
                            innerMin.x = buttonMin.x + padding;
                            innerMax.x = buttonMax.x - padding;
                            break;
                    }

                    switch (row) {
                        case 0: // Top
                            innerMin.y = buttonMin.y + padding;
                            innerMax.y = buttonMin.y + buttonSize.y / 2.0f;
                            break;
                        case 1: // Middle
                            innerMin.y = buttonMin.y + buttonSize.y / 4.0f;
                            innerMax.y = buttonMax.y - buttonSize.y / 4.0f;
                            break;
                        case 2: // Bottom
                            innerMin.y = buttonMin.y + buttonSize.y / 2.0f;
                            innerMax.y = buttonMax.y - padding;
                            break;
                        case 3: // Stretch
                            innerMin.y = buttonMin.y + padding;
                            innerMax.y = buttonMax.y - padding;
                            break;
                    }

                    drawList->AddRectFilled(innerMin, innerMax, IM_COL32(100, 150, 250, 200));
                }
            }

            ImGui::EndTable();
            ImGui::EndPopup();
        }

        if (ImGui::TreeNodeEx("Anchors", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Min");
            if (ImGui::DragFloat2("##AnchorMin", glm::value_ptr(anchor.AnchorMin), 0.01f, 0.0f, 1.0f))
                UIManager::MarkDirty(entity);

            ImGui::Text("Max");
            if (ImGui::DragFloat2("##AnchorMax", glm::value_ptr(anchor.AnchorMax), 0.01f, 0.0f, 1.0f))
                UIManager::MarkDirty(entity);

            ImGui::TreePop();
        }

        bool isStretchingX = anchor.AnchorMin.x != anchor.AnchorMax.x;
        bool isStretchingY = anchor.AnchorMin.y != anchor.AnchorMax.y;

        auto& hierarchyComponent = entity.GetComponent<HierarchyComponent>();
        Entity parentEntity{hierarchyComponent.m_Parent, m_Context.get()};
        auto& parentRenderItem = UIManager::GetUIRenderItem(parentEntity);
        glm::vec2 parentSize = UIManager::GetParentSize(m_Context->m_Registry, parentRenderItem);

        if (!isStretchingX && !isStretchingY)
        {
            glm::vec2 anchoredPos = anchor.GetAnchoredPosition(parentSize);
            ImGui::Text("Position");
            if (ImGui::DragFloat2("##Position", glm::value_ptr(anchoredPos), 1.0f))
            {
                anchor.SetAnchoredPosition(anchoredPos, parentSize);
                UIManager::MarkDirty(entity);
            }

            glm::vec2 size = anchor.GetSize();
            ImGui::Text("Size");
            if (ImGui::DragFloat2("##Size", glm::value_ptr(size), 1.0f, 0.0f, FLT_MAX, "%.0f"))
            {
                anchor.SetSize(size, parentSize);
                UIManager::MarkDirty(entity);
            }
        }

        if (isStretchingX || isStretchingY)
        {
            if (ImGui::TreeNodeEx("Offsets", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (isStretchingX)
                {
                    ImGui::Text("Left");
                    if (ImGui::DragFloat("##OffsetMinX", &anchor.OffsetMin.x, 1.0f))
                        UIManager::MarkDirty(entity);

                    ImGui::Text("Right");
                    if (ImGui::DragFloat("##OffsetMaxX", &anchor.OffsetMax.x, 1.0f))
                        UIManager::MarkDirty(entity);
                }

                if (isStretchingY)
                {
                    ImGui::Text("Top");
                    if (ImGui::DragFloat("##OffsetMinY", &anchor.OffsetMin.y, 1.0f))
                        UIManager::MarkDirty(entity);

                    ImGui::Text("Bottom");
                    if (ImGui::DragFloat("##OffsetMaxY", &anchor.OffsetMax.y, 1.0f))
                        UIManager::MarkDirty(entity);
                }
                ImGui::TreePop();
            }
        }

        float rotation = transformComponent.GetLocalRotation().z;

        ImGui::Text("Rotation");
        if (ImGui::DragFloat("##Rotation", &rotation, 0.1f))
        {
            transformComponent.SetLocalRotation(glm::vec3(0.f, 0.f, rotation));
            UIManager::MarkDirty(entity);
        }
    }

    void SceneTreePanel::DrawComponents(Entity entity)
    {
        if (entity.HasComponent<TagComponent>())
        {
            auto& entityNameTag = entity.GetComponent<TagComponent>().Tag;

            ImGui::Text(ICON_LC_TAG " Tag");
            ImGui::SameLine();

            // Make the tag input field smaller to accommodate the "Static" checkbox
            float availableWidth = ImGui::GetContentRegionAvail().x;
            float tagInputWidth = availableWidth * 0.7f; // Use 70% of the width for the tag input

            // Set the width of the input field
            ImGui::PushItemWidth(tagInputWidth);

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer, entityNameTag.c_str());

            if (ImGui::InputText("##", buffer, sizeof(buffer)))
            {
                entityNameTag = std::string(buffer);
            }

            ImGui::PopItemWidth();

            // Add a small space
            ImGui::SameLine();

            // Add the "Static" checkbox
            bool isStatic = entity.HasComponent<StaticComponent>();
            if (ImGui::Checkbox("Static", &isStatic))
            {
                auto children = entity.GetChildren();
                if (isStatic)
                {
                    if (!entity.HasComponent<StaticComponent>())
                        entity.AddComponent<StaticComponent>();

                    if (children.size() > 0)
                    {
                        // Open a modal to ask if the use wants to apply the static component to all children
                        ImGui::OpenPopup("Apply Static to Children");
                    }
                }
                else
                {
                    if (entity.HasComponent<StaticComponent>())
                        entity.RemoveComponent<StaticComponent>();

                    if (children.size() > 0)
                    {
                        // Open a modal to ask if the use wants to remove the static component to all children
                        ImGui::OpenPopup("Remove Static Children");
                    }
                }
            }

            // Modal for applying static to children
            if (ImGui::BeginPopupModal("Apply Static to Children", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Do you want to apply the static component to all children?");
                ImGui::Separator();
                ImGui::Text("This will make all children static as well.");
                ImGui::Separator();

                if (ImGui::Button("Yes"))
                {
                    std::function<void(Entity)> SetStaticRecursively = [&](Entity currentEntity) {
                        if (!currentEntity.HasComponent<StaticComponent>())
                            currentEntity.AddComponent<StaticComponent>();

                        // Recursively handle children
                        auto children = currentEntity.GetChildren();
                        for (auto& child : children)
                        {
                            SetStaticRecursively(child);
                        }
                    };

                    SetStaticRecursively(entity);

                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginPopupModal("Remove Static Children", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("Do you want to remove the static component from all children?");
                ImGui::Separator();
                ImGui::Text("This will remove static component from all children.");
                ImGui::Separator();

                if (ImGui::Button("Yes"))
                {
                    std::function<void(Entity)> RemoveStaticRecursively = [&](Entity currentEntity) {
                        if (currentEntity.HasComponent<StaticComponent>())
                            currentEntity.RemoveComponent<StaticComponent>();

                        // Recursively handle children
                        auto children = currentEntity.GetChildren();
                        for (auto& child : children)
                        {
                            RemoveStaticRecursively(child);
                        }
                    };

                    RemoveStaticRecursively(entity);

                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("No"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::Separator();
        }

        if (entity.HasComponent<TransformComponent>())
        {
            auto& transformComponent = entity.GetComponent<TransformComponent>();

            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (entity.HasComponent<UIImageComponent>())
                {
                    auto& uiImageComponent = entity.GetComponent<UIImageComponent>();
                    DrawUITransform(transformComponent, uiImageComponent.Anchor, entity);

                    if (ImGui::DragInt("Layer", &uiImageComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else if (entity.HasComponent<UITextComponent>())
                {
                    auto& uiTextComponent = entity.GetComponent<UITextComponent>();
                    DrawUITransform(transformComponent, uiTextComponent.Anchor, entity);
                    if (ImGui::DragInt("Layer", &uiTextComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else if (entity.HasComponent<UIToggleComponent>())
                {
                    auto& uiToggleComponent = entity.GetComponent<UIToggleComponent>();
                    DrawUITransform(transformComponent, uiToggleComponent.Anchor, entity);
                    if (ImGui::DragInt("Layer", &uiToggleComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else if (entity.HasComponent<UIButtonComponent>())
                {
                    auto& uiButtonComponent = entity.GetComponent<UIButtonComponent>();
                    DrawUITransform(transformComponent, uiButtonComponent.Anchor, entity);
                    if (ImGui::DragInt("Layer", &uiButtonComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else if (entity.HasComponent<UISliderComponent>())
                {
                    auto& uiSliderComponent = entity.GetComponent<UISliderComponent>();
                    DrawUITransform(transformComponent, uiSliderComponent.Anchor, entity);
                    if (ImGui::DragInt("Layer", &uiSliderComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else if (entity.HasComponent<UIComponent>())
                {
                    auto& uiComponent = entity.GetComponent<UIComponent>();
                    DrawUITransform(transformComponent, uiComponent.Anchor, entity);
                    if (ImGui::DragInt("Layer", &uiComponent.Layer, 1.0f, 0.0f, 100.0f))
                        UIManager::MarkForSorting();
                }
                else
                {
                    DrawTransform(transformComponent);
                }
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

                if (lightComponent.type == LightComponent::Type::DirectionalLight)
                {
                    ImGui::Text ("Shadow");
                    ImGui::Checkbox("##Shadow", &lightComponent.Shadow);
                    ImGui::Text("Shadow Bias");
                    ImGui::DragFloat("##Shadow Bias", &lightComponent.ShadowBias, 0.001f, 0.0f, 1.0f);
                    ImGui::Text("Shadow Max Distance");
                    ImGui::DragFloat("##Shadow Max Distance", &lightComponent.ShadowMaxDistance, 0.1f);
                }

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
                if(lightComponent.type == LightComponent::Type::SpotLight)
                {
                    ImGui::Text("Angle");
                    ImGui::DragFloat("##Angle", &lightComponent.Angle, 0.1f);

                    ImGui::Text("Cone Attenuation");
                    ImGui::DragFloat("##Cone Attenuation", &lightComponent.ConeAttenuation, 0.1f);
                }
                if (!isCollapsingHeaderOpen)
                {
                    entity.RemoveComponent<LightComponent>();
                }
            }
        }

        if (entity.HasComponent<WorldEnvironmentComponent>())
        {
            auto& worldEnvironmentComponent = entity.GetComponent<WorldEnvironmentComponent>();
            bool isCollapsingHeaderOpen = true;

             auto DrawTextureWidget = [&](const std::string& label, Ref<Cubemap>& texture) {
                 uint32_t textureID = texture ? texture->GetID() : 0;
                 //ImGui::ImageButton(label.c_str(), (ImTextureID)textureID, {64, 64});
                 ImGui::Text("Skybox:");
                 //ImGui::SameLine();
                 ImGui::Button((ICON_LC_CLOUD_SUN + std::string(" ") + (texture ? texture->GetName() : "No Cubemap")).c_str(), {128, 32});

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
                                      // textureImageFormat(texture->GetImageFormat()).c_str(),
                                      texture->GetPath().c_str());
                }

                if (ImGui::BeginDragDropTarget())
                {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                    {
                        const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                        if (resource->GetType() == ResourceType::Cubemap)
                        {
                            const Ref<Cubemap>& t = std::static_pointer_cast<Cubemap>(resource);
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
                            Ref<Cubemap> t = Cubemap::Load(path);
                            texture = t;
                        }
                    }
                    ImGui::EndCombo();
                }
            };

            if (ImGui::CollapsingHeader("World Environment", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if(ImGui::TreeNode("Sky"))
                {
                    DrawTextureWidget("##Cubemap", worldEnvironmentComponent.Skybox);
                    ImGui::Text("Skybox Intensity");
                    ImGui::SameLine();
                    float intensityPlaceholder = 1.0f;
                    ImGui::DragFloat("##Skybox Intensity", /* worldEnvironmentComponent.SkyboxIntensity */&intensityPlaceholder, 0.001f, 100.0f);

                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Tonemap"))
                {
                    ImGui::DragFloat("Exposure", &Renderer3D::GetRenderSettings().Exposure, 0.001f, 100.0f);

                    ImGui::TreePop();
                }
                if(ImGui::TreeNode("Fog"))
                {
                    ImGui::Checkbox("Enable Fog", &worldEnvironmentComponent.Fog);
                    ImGui::DragFloat("Density", &worldEnvironmentComponent.FogDensity, 0.001f, 0.0f, 1.0f);
                    ImGui::ColorEdit3("Color", glm::value_ptr(worldEnvironmentComponent.FogColor));
                    ImGui::DragFloat("Height", &worldEnvironmentComponent.FogHeight, 0.1f, -1024.0f, 1024.0f);
                    ImGui::DragFloat("Height Density", &worldEnvironmentComponent.FogHeightDensity, 0.001f, -16.0f, 16.0f);

                    ImGui::TreePop();
                }
                if (ImGui::TreeNode("Bloom"))
                {
                    ImGui::Checkbox("Enable Bloom", &worldEnvironmentComponent.Bloom);
                    ImGui::DragFloat("Intensity", &worldEnvironmentComponent.BloomIntensity, 0.001f, 0.0f, 10.0f);
                    ImGui::DragFloat("Radius", &worldEnvironmentComponent.BloomRadius, 0.001f, 0.0f, 5.0f);
                    ImGui::DragInt("Max Mip Levels", &worldEnvironmentComponent.BloomMaxMipLevels, 1, 1, 10);

                    ImGui::TreePop();
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
            // TODO Move this function to ImGuiExtras.cpp, this is duplicated in World Environment
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
                                        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview);
                    ImGui::EndPopup();
                }
            };

            static bool ShowShaderQuickLoad = false;

            auto& materialComponent = entity.GetComponent<MaterialComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Material", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                // Check if the material is valid
                if (!materialComponent.material)
                {
                    // If the material is not valid render a button to choose a new one

                    static bool MaterialSelectorOpen = false;

                    ImGui::Text("Material");
                    ImGui::SameLine();
                    if (ImGui::Button("<Empty>", {64, 32}))
                    {
                        MaterialSelectorOpen = true;
                    }

                    if (MaterialSelectorOpen)
                    {
                        ImGui::OpenPopup("MaterialPopup");
                        MaterialSelectorOpen = false;
                    }

                    if (ImGui::BeginPopup("MaterialPopup"))
                    {
                        if (ImGui::MenuItem(ICON_LC_BOX "PBRMaterial"))
                        {
                            materialComponent.material = PBRMaterial::Create();
                            materialComponent.material->SetEmbedded(true);
                        }
                        if (ImGui::MenuItem(ICON_LC_SQUARE_CHART_GANTT "ShaderMaterial"))
                        {
                            materialComponent.material = ShaderMaterial::Create();
                            materialComponent.material->SetEmbedded(true);
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem(ICON_LC_FOLDER "Quick Load...", NULL, false, false))
                        {
                        }
                        {
                        }
                        if (ImGui::MenuItem(ICON_LC_FOLDER "Load...", NULL, false, false))
                        {
                            /*std::string path = FileDialog::OpenFile({}).string();
                            if (!path.empty())
                            {
                            }
                            */
                        }
                        ImGui::EndPopup();
                    }

                    if (!isCollapsingHeaderOpen)
                    {
                        entity.RemoveComponent<MaterialComponent>();
                    }
                    return;
                }

                if (materialComponent.material->GetType() == ResourceType::PBRMaterial)
                {
                    PBRMaterial& pbrMaterial = *std::static_pointer_cast<PBRMaterial>(materialComponent.material);

                    PBRMaterialTextures& materialTextures = pbrMaterial.GetTextures();
                    PBRMaterialProperties& materialProperties = pbrMaterial.GetProperties();
                    MaterialRenderSettings& materialRenderSettings = pbrMaterial.GetRenderSettings();

                    if (ImGui::TreeNode("Render Settings"))
                    {
                        ImGui::BeginChild("##RenderSettings Child", {0, 0},
                                            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

                        ImGui::Text("Transparency");
                        ImGui::SameLine();
                        ImGui::Combo("##Transparency", (int*)&materialRenderSettings.transparencyMode,
                                        "Disabled\0Alpha\0AlphaCutoff\0");

                        if (materialRenderSettings.transparencyMode == MaterialRenderSettings::TransparencyMode::AlphaCutoff)
                        {
                            ImGui::Text("Alpha Cutoff");
                            ImGui::SameLine();
                            ImGui::SliderFloat("##AlphaCutoff", &materialRenderSettings.alphaCutoff, 0.0f, 1.0f);
                        }

                        ImGui::Text("Cull Mode");
                        ImGui::SameLine();
                        ImGui::Combo("##CullMode", (int*)&materialRenderSettings.cullMode,
                                        "Front\0Back\0None\0");

                        ImGui::Text("Depth Test");
                        ImGui::SameLine();
                        ImGui::Checkbox("##DepthTest", &materialRenderSettings.depthTest);

                        ImGui::Text("Wireframe");
                        ImGui::SameLine();
                        ImGui::Checkbox("##Wireframe", &materialRenderSettings.wireframe);

                        ImGui::EndChild();
                        ImGui::TreePop();
                    }

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
                        ImGui::SliderFloat("##Roughness Slider", &materialProperties.roughness, 0.0f, 1.0f);
                        ImGui::Text("Texture");
                        DrawTextureWidget("##Roughness", materialTextures.roughness);
                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    if (ImGui::TreeNode("Emission"))
                    {
                        ImGui::BeginChild("##Emission Child", {0, 0},
                                            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);
                        
                        ImGui::Text("Color");
                        glm::vec4 emissiveColor = glm::vec4(materialProperties.emissive, 1.0f);
                        
                        // Use ColorEdit4 with HDR flag to allow values > 1.0
                        if (ImGui::ColorEdit4("##Emissive Color", glm::value_ptr(emissiveColor), 
                                              ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float))
                        {
                            materialProperties.emissive = glm::vec3(emissiveColor);
                        }
                        
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
                else if (materialComponent.material->GetType() == ResourceType::ShaderMaterial)
                {
                    ShaderMaterial& shaderMaterial = *std::static_pointer_cast<ShaderMaterial>(materialComponent.material);
                    MaterialRenderSettings& materialRenderSettings = shaderMaterial.GetRenderSettings();

                    if (ImGui::TreeNode("Render Settings"))
                    {
                        ImGui::BeginChild("##RenderSettings Child", {0, 0},
                                            ImGuiChildFlags_AutoResizeY | ImGuiChildFlags_Borders);

                        ImGui::Text("Transparency");
                        ImGui::SameLine();
                        ImGui::Combo("##Transparency", (int*)&materialRenderSettings.transparencyMode,
                                        "Disabled\0Alpha\0AlphaCutoff\0");

                        if (materialRenderSettings.transparencyMode == MaterialRenderSettings::TransparencyMode::AlphaCutoff)
                        {
                            ImGui::Text("Alpha Cutoff");
                            ImGui::SameLine();
                            ImGui::SliderFloat("##AlphaCutoff", &materialRenderSettings.alphaCutoff, 0.0f, 1.0f);
                        }

                        ImGui::Text("Cull Mode");
                        ImGui::SameLine();
                        ImGui::Combo("##CullMode", (int*)&materialRenderSettings.cullMode,
                                        "Front\0Back\0None\0");

                        ImGui::Text("Depth Test");
                        ImGui::SameLine();
                        ImGui::Checkbox("##DepthTest", &materialRenderSettings.depthTest);

                        ImGui::Text("Wireframe");
                        ImGui::SameLine();
                        ImGui::Checkbox("##Wireframe", &materialRenderSettings.wireframe);

                        ImGui::EndChild();
                        ImGui::TreePop();
                    }
                    ImGui::Text("Shader");
                    ImGui::SameLine();
                    std::string shaderName = shaderMaterial.GetShader() ? shaderMaterial.GetShader()->GetName() : "<Empty>";

                    static bool LoadShaderPopupOpen = false;

                    if (ImGui::Button(shaderName.c_str(), {64, 32}))
                    {
                        if (!shaderMaterial.GetShader())
                        {
                            LoadShaderPopupOpen = true;
                        }
                        else
                        {
                            SDL_OpenURL(("file://" + std::filesystem::absolute(shaderMaterial.GetShader()->GetPath()).string()).c_str());
                        }
                    }
                    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
                    {
                        LoadShaderPopupOpen = true;
                    }

                    if (LoadShaderPopupOpen)
                    {
                        ImGui::OpenPopup("LoadShaderPopup");
                        LoadShaderPopupOpen = false;
                    }

                    if(ImGui::BeginPopup("LoadShaderPopup"))
                    {
                        if (ImGui::MenuItem(ICON_LC_SQUARE_CHART_GANTT "New Shader..."))
                        {
                            std::string path = FileDialog::SaveFile({}).string();
                            if (!path.empty())
                            {
                                std::ofstream scriptFile(path);
                                std::ifstream defaultShaderFile("assets/shaders/DefaultCustomShader.glsl");
                                if (scriptFile.is_open() and defaultShaderFile.is_open())
                                {
                                    scriptFile << defaultShaderFile.rdbuf();
                                    scriptFile.close();
                                    defaultShaderFile.close();
                                }
                                Ref<Shader> shader = Shader::Create(path);
                                shaderMaterial.SetShader(shader);
                            }
                        }
                        ImGui::Separator();
                        if (ImGui::MenuItem(ICON_LC_FOLDER "Quick Load..."))
                        {
                            ShowShaderQuickLoad = true;
                        }
                        if (ImGui::MenuItem(ICON_LC_FOLDER "Load..."))
                        {
                            std::string path = FileDialog::OpenFile({}).string();
                            if (!path.empty())
                            {
                                Ref<Shader> shader = Shader::Create(path);
                                shaderMaterial.SetShader(shader);
                            }
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginDragDropTarget())
                    {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("RESOURCE"))
                        {
                            const Ref<Resource>& resource = *(Ref<Resource>*)payload->Data;
                            if (resource->GetType() == ResourceType::Shader)
                            {
                                const Ref<Shader>& t = std::static_pointer_cast<Shader>(resource);
                                shaderMaterial.SetShader(t);
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                    if (ImGui::Button("Recompile"))
                    {
                        if (shaderMaterial.GetShader()) shaderMaterial.GetShader()->Recompile();
                    }

                    if (!isCollapsingHeaderOpen)
                    {
                        entity.RemoveComponent<MaterialComponent>();
                    }
                }
                if(materialComponent.material and ImGui::TreeNode("Resource"))
                {
                    bool embedded = materialComponent.material->IsEmbedded();
                    if (ImGui::Checkbox("Embedded", &embedded))
                    {
                        materialComponent.material->SetEmbedded(embedded);
                    }

                    ImGui::Text("Path: %s", materialComponent.material->GetPath().string().c_str());
                    ImGui::Text("UUID: %s", std::to_string(materialComponent.material->GetUUID()).c_str());

                    ImGui::TreePop();
                }
            }

            if (ShowShaderQuickLoad)
            {
                ImGui::OpenPopup("ShaderQuickLoad");
                ShowShaderQuickLoad = false;
            }
            if (ImGui::BeginPopupModal("ShaderQuickLoad"))
            {
                ImGui::Text("Quick Load");
                ImGui::Separator();
                auto& registry = ResourceRegistry::GetResourceRegistry();
                for (auto& shader : registry)
                {
                    if (shader.second->GetType() == ResourceType::Shader)
                    {
                        const Ref<Shader>& t = std::static_pointer_cast<Shader>(shader.second);
                        if (ImGui::Selectable(t->GetName().c_str()))
                        {
                            auto shaderMaterial = std::static_pointer_cast<ShaderMaterial>(materialComponent.material);
                            shaderMaterial->SetShader(t);
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                if(ImGui::Button("Close"))
                {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
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
                    int colliderType = -1; // -1: Unknown, 0: Box, 1: Sphere, 2: Capsule, 3: Cone, 4: Cylinder
                    
                    if (currentCollider) {
                        if (std::dynamic_pointer_cast<BoxCollider>(currentCollider)) {
                            colliderType = 0;
                        } else if (std::dynamic_pointer_cast<SphereCollider>(currentCollider)) {
                            colliderType = 1;
                        } else if (std::dynamic_pointer_cast<CapsuleCollider>(currentCollider)) {
                            colliderType = 2;
                        } else if (std::dynamic_pointer_cast<ConeCollider>(currentCollider)) {
                            colliderType = 3;
                        } else if (std::dynamic_pointer_cast<CylinderCollider>(currentCollider)) {
                            colliderType = 4;
                        }
                    }
                    
                    static const char* colliderTypeNames[] = { "Box", "Sphere", "Capsule", "Cone", "Cylinder" };
                    int newColliderType = colliderType;
                    
                    if (ImGui::Combo("Type##ColliderType", &newColliderType, colliderTypeNames, 5)) {
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
                            case 3: { // Cone
                                float radius = 0.5f;
                                float height = 1.0f;
                                if (auto coneCollider = std::dynamic_pointer_cast<ConeCollider>(currentCollider)) {
                                    radius = coneCollider->GetRadius();
                                    height = coneCollider->GetHeight();
                                }
                                newCollider = CreateRef<ConeCollider>(radius, height);
                                break;
                            }
                            case 4: { // Cylinder
                                float radius = 0.5f;
                                float height = 1.0f;
                                if (auto cylinderCollider = std::dynamic_pointer_cast<CylinderCollider>(currentCollider)) {
                                    radius = cylinderCollider->GetRadius();
                                    height = cylinderCollider->GetHeight();
                                }
                                newCollider = CreateRef<CylinderCollider>(radius, height);
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

                                        newCollider->setOffset(currentCollider->getOffset());

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

                                        newCollider->setOffset(currentCollider->getOffset());

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

                                        newCollider->setOffset(currentCollider->getOffset());

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
                            case 3: { // Cone collider properties
                                auto coneCollider = std::dynamic_pointer_cast<ConeCollider>(currentCollider);
                                if (coneCollider) {
                                    float radius = coneCollider->GetRadius();
                                    float height = coneCollider->GetHeight();

                                    ImGui::Text("Radius");
                                    bool radiusChanged = ImGui::DragFloat("##ConeRadius", &radius, 0.1f, 0.01f, 100.0f);

                                    ImGui::Text("Height");
                                    bool heightChanged = ImGui::DragFloat("##ConeHeight", &height, 0.1f, 0.01f, 100.0f);

                                    if (radiusChanged || heightChanged) {
                                        // Create new cone collider with updated parameters
                                        Ref<Collider> newCollider = CreateRef<ConeCollider>(radius, height);

                                        newCollider->setOffset(currentCollider->getOffset());

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
                            case 4: { // Cylinder collider properties
                                auto cylinderCollider = std::dynamic_pointer_cast<CylinderCollider>(currentCollider);
                                if (cylinderCollider) {
                                    float radius = cylinderCollider->GetRadius();
                                    float height = cylinderCollider->GetHeight();

                                    ImGui::Text("Radius");
                                    bool radiusChanged = ImGui::DragFloat("##CylinderRadius", &radius, 0.1f, 0.01f, 100.0f);

                                    ImGui::Text("Height");
                                    bool heightChanged = ImGui::DragFloat("##CylinderHeight", &height, 0.1f, 0.01f, 100.0f);

                                    if (radiusChanged || heightChanged) {
                                        // Create new cylinder collider with updated parameters
                                        Ref<Collider> newCollider = CreateRef<CylinderCollider>(radius, height);

                                        newCollider->setOffset(currentCollider->getOffset());

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

                    ImGui::Text("Collider Offset");
                    glm::vec3 offset = currentCollider->getOffset();
                    if (ImGui::DragFloat3("##ColliderOffset", glm::value_ptr(offset), 0.1f))
                    {
                        // Store current rigidbody properties before modifying
                        RigidBody::Properties props = rbComponent.rb->GetProperties();
                        glm::vec3 position = rbComponent.rb->GetPosition();
                        glm::vec3 rotation = rbComponent.rb->GetRotation();
                        glm::vec3 velocity = rbComponent.rb->GetVelocity();

                        // Remove from physics world
                        m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                        // Update the collider offset
                        currentCollider->setOffset(offset);

                        // Create new rigidbody with the updated collider
                        rbComponent.rb = RigidBody::Create(props, currentCollider);
                        rbComponent.rb->SetPosition(position);
                        rbComponent.rb->SetRotation(rotation);
                        rbComponent.rb->SetVelocity(velocity);

                        // Add back to physics world
                        m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                        rbComponent.rb->GetNativeBody()->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));
                    }

                    if (ImGui::Button("Resize Collider to Fit Mesh AABB", ImVec2(-FLT_MIN, 0))) {
                        if (!ResizeColliderToFitMeshAABB(entity, rbComponent)) {
                            // Display error messages only if resize failed
                            if (!entity.HasComponent<MeshComponent>()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Entity has no mesh component!");
                            } else if (!entity.GetComponent<MeshComponent>().GetMesh()) {
                                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No valid mesh found!");
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
                    rbComponent.rb->GetNativeBody()->setUserPointer(nullptr); // Set user pointer to null to avoid dangling references
                    rbComponent.rb.reset();
                }
                entity.RemoveComponent<RigidbodyComponent>();
            }
        }

        if (entity.HasComponent<AnimatorComponent>())
        {
            auto& animatorComponent = entity.GetComponent<AnimatorComponent>();

            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Animator", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                const char* UpperAnimName = animatorComponent.GetAnimationController()->GetAnimation(animatorComponent.UpperAnimation->CurrentAnimation)->GetAnimationName().c_str();
                const char* LowerAnimName = animatorComponent.GetAnimationController()->GetAnimation(animatorComponent.LowerAnimation->CurrentAnimation)->GetAnimationName().c_str();
                const char* AnimName = UpperAnimName == LowerAnimName ? UpperAnimName : "Mixed";

                if (ImGui::BeginCombo("Animation", AnimName))
                {
                    for (auto& [name, animation] : animatorComponent.GetAnimationController()->GetAnimationMap())
                    {
                        if (ImGui::Selectable(name.c_str()) && name != AnimName)
                        {
                            UpperAnimName = name.c_str();
                            LowerAnimName = name.c_str();
                            animatorComponent.SetCurrentAnimation(animation);
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("UpperAnimation", UpperAnimName))
                {
                    for (auto& [name, animation] : animatorComponent.GetAnimationController()->GetAnimationMap())
                    {
                        if (ImGui::Selectable(name.c_str()) && name != UpperAnimName)
                        {;
                            UpperAnimName = name.c_str();
                            animatorComponent.SetUpperAnimation(animation);
                        }
                    }
                    ImGui::EndCombo();
                }

                if (ImGui::BeginCombo("LowerAnimation", LowerAnimName))
                {
                    for (auto& [name, animation] : animatorComponent.GetAnimationController()->GetAnimationMap())
                    {
                        if (ImGui::Selectable(name.c_str()) && name != LowerAnimName)
                        {
                            LowerAnimName = name.c_str();
                            animatorComponent.SetLowerAnimation(animation);
                        }
                    }
                    ImGui::EndCombo();
                }

                const char* RootJointName = animatorComponent.GetSkeleton()->GetJoints()[animatorComponent.UpperBodyRootJoint].name.c_str();

                if (ImGui::BeginCombo("RootJointName", RootJointName))
                {
                    for (auto& joint : animatorComponent.GetSkeleton()->GetJoints())
                    {
                        if (ImGui::Selectable(joint.name.c_str()) && joint.name != RootJointName)
                        {
                            RootJointName = joint.name.c_str();
                            std::map<std::string, unsigned int> animationMap = animatorComponent.GetAnimationController()->GetAnimationMap();
                            AnimationSystem::SetupPartialBlending(animationMap[UpperAnimName], animationMap[LowerAnimName], RootJointName, &animatorComponent);
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::DragFloat("Blend Duration", &animatorComponent.BlendDuration, 0.01f, 0.01f, 2.0f, "%.2f");

                ImGui::DragFloat("UpperBodyWeight", &animatorComponent.UpperBodyWeight, 0.01f, 0.0f, 1.0f, "%.2f");

                ImGui::DragFloat("LowerBodyWeight", &animatorComponent.LowerBodyWeight, 0.01f, 0.0f, 1.0f, "%.2f");

                ImGui::DragFloat("PartialBlendThreshold", &animatorComponent.PartialBlendThreshold, 0.01f, 0.0f, 1.0f, "%.2f");

                ImGui::DragFloat("Animation Speed", &animatorComponent.AnimationSpeed, 0.01f, 0.1f, 5.0f, "%.2f");

                ImGui::Checkbox("Loop", &animatorComponent.Loop);

                animatorComponent.NeedsUpdate = true;
            }

            if (!isCollapsingHeaderOpen)
            {
                // entity.RemoveComponent<AnimatorComponent>();
                // TODO remove animator component from entity and all the animation data
            }
        }
        
        if(entity.HasComponent<ScriptComponent>())
        {
            auto& scriptComponent = entity.GetComponent<ScriptComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Script", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Text(scriptComponent.script->GetPath().filename().string().c_str());

                if (ImGui::Button("Open in Editor"))
                {
                    SDL_OpenURL(("file://" + std::filesystem::absolute(scriptComponent.script->GetPath()).string()).c_str());
                }

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
                bool showComponent = navMeshComponent.ShowDebug;
                if (ImGui::Checkbox("Show NavMesh", &showComponent))
                {
                    navMeshComponent.ShowDebug = showComponent;

                    if (showComponent)
                        m_Context->m_SceneDebugFlags.ShowNavMesh = true;
                }

                ImGui::DragFloat("Walkable Slope Angle", &navMeshComponent.GetNavMesh()->WalkableSlopeAngle, 0.1f, 0.1f, 60.0f);

                if (ImGui::SmallButton("Generate NavMesh"))
                {
                    navMeshComponent.GetNavMesh()->CalculateWalkableAreas(entity.GetComponent<MeshComponent>().GetMesh(), entity.GetComponent<TransformComponent>().GetWorldTransform());
                }
            }
            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<NavMeshComponent>();
            }
        }

        if (entity.HasComponent<NavigationAgentComponent>())
        {
            auto& navigationAgentComponent = entity.GetComponent<NavigationAgentComponent>();
            bool isCollapsingHeaderOpen = true;
            if (ImGui::CollapsingHeader("Navigation Agent", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto view = m_Context->m_Registry.view<NavMeshComponent>();

                bool showComponent = navigationAgentComponent.ShowDebug;
                if (ImGui::Checkbox("Show Path", &showComponent))
                {
                    navigationAgentComponent.ShowDebug = showComponent;

                    if (showComponent)
                        m_Context->m_SceneDebugFlags.ShowNavMeshPath = true;
                }

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

        if (entity.HasComponent<SpriteComponent>())
        {
            auto& spriteComponent = entity.GetComponent<SpriteComponent>();
            bool isCollapsingHeaderOpen = true;
            ImGui::PushID("SpriteComponent");
            if (ImGui::CollapsingHeader("Sprite Component", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                auto DrawTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture) {
                    uint32_t textureID = texture ? texture->GetID() : 0;
                    ImGui::ImageButton((label + " ").c_str(), (ImTextureID)textureID, {64, 64});

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
                    if (ImGui::BeginCombo((label).c_str(), "", ImGuiComboFlags_NoPreview))
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

                DrawTextureWidget("Texture 2D", spriteComponent.texture);

                ImGui::ColorEdit4("Tint Color", glm::value_ptr(spriteComponent.tintColor));
                ImGui::DragFloat("Tilling Factor", &spriteComponent.tilingFactor, 0.1, 0);

                ImGui::Checkbox("Flip X", &spriteComponent.flipX);
                ImGui::Checkbox("Flip Y", &spriteComponent.flipY);
            }
            ImGui::PopID();

        }

        if (entity.HasComponent<ParticlesSystemComponent>())
        {
            auto& particles = entity.GetComponent<ParticlesSystemComponent>();
            particles.NeedsUpdate = true;
            Ref<ParticleEmitter> emitter = particles.GetParticleEmitter();
            bool isCollapsingHeaderOpen = true;

            ImGui::PushID("ParticlesSystem");
            if (ImGui::CollapsingHeader("Particle System", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {

                ImGui::Text("Gravity");
                ImGui::DragFloat3("##ParticleGravity", glm::value_ptr(emitter->gravity), 0.01f);

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

                //ImGui::Checkbox("##UseEmission", &emitter->useEmission);
                //ImGui::SameLine();
                ImGui::PushID("Emission");

                if (ImGui::TreeNodeEx("Emission Settings", ImGuiTreeNodeFlags_None))
                {

                    // Select emitter shape
                    ImGui::Text("Rate over Time");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticleRateOverTime", &emitter->rateOverTime, 0.1, 0);

                    ImGui::Text("Emit test");
                    ImGui::SameLine();
                    ImGui::DragFloat("##ParticlesEmitTest", &emitter->emitParticlesTest, 0.1, 0);
                    if (ImGui::Button("Emit Particles"))
                    {
                        emitter->Emit(emitter->emitParticlesTest);
                    }

                    ImGui::Checkbox("##UseBurst", &emitter->useBurst);
                    ImGui::SameLine();
                    if (ImGui::TreeNodeEx("Bursts Settings", ImGuiTreeNodeFlags_None))
                    {
                        if (!emitter->useBurst)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                        }
                        ImGui::PushItemWidth(50.0f);

                        ImGui::Text("Time  ");
                        ImGui::SameLine();
                        ImGui::Text("Count  ");
                        ImGui::SameLine();
                        ImGui::Text("Interval");


                        for (int i = 0; i < emitter->bursts.size(); i++)
                        {
                            Ref<BurstParticleEmitter> burst = emitter->bursts[i];

                            std::string number = std::to_string(i);

                            ImGui::DragFloat(("##Time" + number).c_str(), &burst->initialTime, 0.1, 0);
                            ImGui::SameLine();
                            ImGui::DragInt(("##Count" + number).c_str(), &burst->count, 1, 0);
                            ImGui::SameLine();
                            ImGui::DragFloat(("##Interval" + number).c_str(), &burst->interval, 0.01f, 0);

                            ImGui::SameLine();
                            if (ImGui::Button(("X##" + number).c_str()))
                            {
                                emitter->bursts.erase(emitter->bursts.begin() + i);
                                i--;
                            }

                        }


                        if (ImGui::Button("Add Burst"))
                        {
                            Ref<BurstParticleEmitter> newBurst = CreateRef<BurstParticleEmitter>();
                            newBurst->initialTime = 0.0f;
                            newBurst->count = 0.0f;
                            newBurst->interval = 0.0f;

                            emitter->bursts.push_back(newBurst);
                        }




                        if (!emitter->useBurst)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleColor();
                        }
                        ImGui::PopItemWidth();

                        ImGui::TreePop();
                    }


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
                    const char* shapeNames[] = {"Circle", "Cone", "Box"};
                    ImGui::Text("Shape");
                    ImGui::SameLine();
                    ImGui::Combo("##ShapeType", reinterpret_cast<int*>(&emitter->shape), shapeNames,
                                 IM_ARRAYSIZE(shapeNames));


                    if (emitter->shape == ParticleEmitter::ShapeType::Box ||
                        emitter->shape == ParticleEmitter::ShapeType::Circle)
                    {
                        // Direction
                        ImGui::Checkbox("Go Center Direction", &emitter->goCenter);

                        if (emitter->goCenter)
                        {
                            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f)); // Gray
                            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);                   // Disable controls
                        }

                        ImGui::Checkbox("##ParticleDirectionUseRandom", &emitter->useDirectionRandom);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip(
                                "Check this button to use the random system.\nThe value above is the min, and "
                                "the value below is the max.");
                        }
                        ImGui::SameLine();
                        ImGui::Text("Direction");
                        ImGui::DragFloat3("##ParticleDirectionNormal", glm::value_ptr(emitter->direction), 0.1f, -1.0f,
                                          1.0f);
                        if (emitter->useDirectionRandom)
                        {
                            ImGui::DragFloat3("##ParticleDirectionRandom", glm::value_ptr(emitter->directionRandom),
                                              0.1f, -1.0f, 1.0f);
                        }
                        if (emitter->goCenter)
                        {
                            ImGui::PopItemFlag();
                            ImGui::PopStyleColor();
                        }
                    }



                    if (emitter->shape == ParticleEmitter::ShapeType::Box)
                    {
                        ImGui::Text("Spread");
                        ImGui::DragFloat3("##ParticleSpreadMin", glm::value_ptr(emitter->minSpread), 0.1f);
                        ImGui::DragFloat3("##ParticleSpreadMax", glm::value_ptr(emitter->maxSpread), 0.1f);
                    }


                    if (emitter->shape == ParticleEmitter::ShapeType::Cone)
                    {
                        ImGui::Text("Radius");
                        ImGui::SameLine();
                        ImGui::DragFloat("##Radius", &emitter->shapeRadius, 0.001f, 0.001f);

                        ImGui::Text("Angle");
                        ImGui::SameLine();
                        ImGui::DragFloat("##Angle", &emitter->shapeAngle, 0.001f, 0.1f, 1.5f);
                        if (emitter->shapeAngle <= 0.099)
                        {
                            emitter->shapeAngle = 0.1f;
                        }
                    }

                    if (emitter->shape == ParticleEmitter::ShapeType::Circle)
                    {

                        // Control the radius
                        ImGui::Text("Radius");
                        ImGui::SameLine();
                        ImGui::DragFloat("##Radius", &emitter->shapeRadius, 0.001f, 0.0f);

                        ImGui::Text("Radius Thickness");
                        ImGui::SameLine();
                        ImGui::DragFloat("##RadiusThickness", &emitter->shapeRadiusThickness, 0.001f, 0.0f,
                                         emitter->shapeRadius);
                        if (emitter->shapeRadiusThickness > emitter->shapeRadius)
                                emitter->shapeRadiusThickness = emitter->shapeRadius;
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

                    ImGui::DragFloat("Velocity Multiplier", &emitter->velocityMultiplier, 0.1f);

                    ImGui::Checkbox("Separate Axes", &emitter->velocityOverLifeTimeSeparateAxes);

                    if (emitter->velocityOverLifeTimeSeparateAxes)
                    {
                        ImGui::Text("Velocity X");
                        if (CurveEditor::DrawCurve("Velocity X", emitter->speedOverLifeTimeX))
                            emitter->InvalidateCurves();

                        ImGui::Text("Velocity Y ");
                        if (CurveEditor::DrawCurve("Velocity Y", emitter->speedOverLifeTimeY))
                            emitter->InvalidateCurves();

                        ImGui::Text("Velocity Z");
                        if (CurveEditor::DrawCurve("Velocity Z", emitter->speedOverLifeTimeZ))
                            emitter->InvalidateCurves();
                    }
                    else
                    {
                        ImGui::Text("Velocity");
                        if (CurveEditor::DrawCurve("Velocity", emitter->speedOverLifeTimeGeneral))
                            emitter->InvalidateCurves();
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

                    if (GradientEditor::ShowGradientEditor(emitter->colorOverLifetime_gradientPoints))
                        emitter->InvalidateCurves();

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

                    ImGui::DragFloat("Size Multiplier", &emitter->sizeMultiplier, 0.1f);

                    // Enable or disable separate XYZ axes
                    ImGui::Checkbox("Separate Axes", &emitter->sizeOverLifeTimeSeparateAxes);

                    if (emitter->sizeOverLifeTimeSeparateAxes)
                    {
                        ImGui::Text("Size X");
                        if (CurveEditor::DrawCurve("Size X", emitter->sizeOverLifetimeX))
                            emitter->InvalidateCurves();

                        ImGui::Text("Size Y");
                        if (CurveEditor::DrawCurve("Size Y", emitter->sizeOverLifetimeY))
                            emitter->InvalidateCurves();

                        ImGui::Text("Size Z");
                        if (CurveEditor::DrawCurve("Size Z", emitter->sizeOverLifetimeZ))
                            emitter->InvalidateCurves();
                    }
                    else
                    {
                        ImGui::Text("Size");
                        if (CurveEditor::DrawCurve("Size", emitter->sizeOverLifetimeGeneral))
                            emitter->InvalidateCurves();
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

                    ImGui::DragFloat("Rotation Multiplier", &emitter->rotationMultiplier, 0.1f);

                    // Rotation on X axis
                    ImGui::Text("Rotation X");
                    if (CurveEditor::DrawCurve("##RotationX", emitter->rotationOverLifetimeX))
                        emitter->InvalidateCurves();

                    // Rotation on Y axis
                    ImGui::Text("Rotation Y");
                    if (CurveEditor::DrawCurve("##RotationY", emitter->rotationOverLifetimeZ))
                        emitter->InvalidateCurves();

                    // Rotation on Z axis
                    ImGui::Text("Rotation Z");
                    if (CurveEditor::DrawCurve("##RotationZ", emitter->rotationOverLifetimeY))
                        emitter->InvalidateCurves();

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

                    // Simulation Space Mode selection
                    const char* simulationModes[] = {"Local", "World"};
                    ImGui::Text("Simulation Space");
                    ImGui::SameLine();
                    ImGui::Combo("##SimulationSpace", reinterpret_cast<int*>(&emitter->simulationSpace),
                                 simulationModes, IM_ARRAYSIZE(simulationModes));



                    // Material selection
                    ImGui::Text("Material");
                    auto DrawTextureWidget = [&](const std::string& label, Ref<Texture2D>& texture) {
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

                    ImGui::Text("Texture");
                    DrawTextureWidget("##Albedo", emitter->particleTexture);


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

        if (entity.HasComponent<UIComponent>())
        {
            bool isCollapsingHeaderOpen = true;
            ImGui::CollapsingHeader("UI Empty Component", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen);

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UIComponent>();
            }
        }

        if (entity.HasComponent<UIImageComponent>())
        {
            auto& imageComponent = entity.GetComponent<UIImageComponent>();
            bool isCollapsingHeaderOpen = true;

            if (ImGui::CollapsingHeader("UI Image", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (ImGui::Selectable("Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        imageComponent.SetTexture(texture);
                    }
                }

                ImGui::ColorEdit4("Color", glm::value_ptr(imageComponent.Color));

                ImGui::DragFloat4("UVRect", glm::value_ptr(imageComponent.UVRect), 0.01f, 0.0f, 1.0f);
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UIImageComponent>();
            }
        }

        if (entity.HasComponent<UITextComponent>())
        {
            auto& textComponent = entity.GetComponent<UITextComponent>();
            bool isCollapsingHeaderOpen = true;

            if (ImGui::CollapsingHeader("UI Text", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                char buffer[256] = {};

                strncpy(buffer, textComponent.Text.c_str(), sizeof(buffer));

                if (ImGui::InputTextMultiline("##Text", buffer, sizeof(buffer)))
                {
                    textComponent.Text = std::string(buffer);
                }

                if (ImGui::Selectable("Font"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        textComponent.FontPath = path;
                        textComponent.UIFont = CreateRef<Font>(path);
                    }
                }

                const char* alignmentNames[] = {"Left", "Center", "Right"};
                ImGui::Combo("TextAlignment", reinterpret_cast<int*>(&textComponent.Alignment), alignmentNames, IM_ARRAYSIZE(alignmentNames));

                ImGui::DragFloat("Size", &textComponent.FontSize, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Kerning", &textComponent.Kerning, 0.1f, 0.0f, 100.0f);
                ImGui::DragFloat("Line Spacing", &textComponent.LineSpacing, 0.1f, 0.0f, 100.0f);
                ImGui::ColorEdit4("Color", glm::value_ptr(textComponent.Color));
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UITextComponent>();
            }
        }

        if (entity.HasComponent<UIToggleComponent>())
        {
            auto& toggleComponent = entity.GetComponent<UIToggleComponent>();
            bool isCollapsingHeaderOpen = true;

            if (ImGui::CollapsingHeader("Toggle", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Value", &toggleComponent.Value);

                if (ImGui::Selectable("On Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        toggleComponent.OnTexture = texture;
                    }
                }

                if (ImGui::Selectable("Off Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        toggleComponent.OffTexture = texture;
                    }
                }
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UIToggleComponent>();
            }
        }

        if (entity.HasComponent<UIButtonComponent>())
        {
            auto& buttonComponent = entity.GetComponent<UIButtonComponent>();
            bool isCollapsingHeaderOpen = true;

            if (ImGui::CollapsingHeader("UI Button", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::Checkbox("Interactable", &buttonComponent.Interactable);

                if (ImGui::Selectable("Normal Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        buttonComponent.NormalTexture = texture;
                    }
                }

                if (ImGui::Selectable("Hover Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        buttonComponent.HoverTexture = texture;
                    }
                }

                if (ImGui::Selectable("Pressed Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        buttonComponent.PressedTexture = texture;
                    }
                }

                if (ImGui::Selectable("Disabled Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        buttonComponent.DisabledTexture = texture;
                    }
                }

                ImGui::Text("Normal Color");
                ImGui::ColorEdit4("##NormalColor", glm::value_ptr(buttonComponent.NormalColor));

                ImGui::Text("Hover Color");
                ImGui::ColorEdit4("##HoverColor", glm::value_ptr(buttonComponent.HoverColor));

                ImGui::Text("Pressed Color");
                ImGui::ColorEdit4("##PressedColor", glm::value_ptr(buttonComponent.PressedColor));

                ImGui::Text("Disabled Color");
                ImGui::ColorEdit4("##DisabledColor", glm::value_ptr(buttonComponent.DisabledColor));

                const char* stateNames[] = { "Normal", "Hover", "Pressed", "Disabled" };
                ImGui::Text("Current State: %s", stateNames[static_cast<int>(buttonComponent.CurrentState)]);
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UIButtonComponent>();
            }
        }

        if (entity.HasComponent<UISliderComponent>())
        {
            auto& sliderComponent = entity.GetComponent<UISliderComponent>();
            bool isCollapsingHeaderOpen = true;

            if (ImGui::CollapsingHeader("UI Slider", &isCollapsingHeaderOpen, ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::DragFloat("Value", &sliderComponent.Value, 0.1f, sliderComponent.MinValue, sliderComponent.MaxValue);
                ImGui::DragFloat("Min Value", &sliderComponent.MinValue, 0.1f, 0.0f, sliderComponent.MaxValue);
                ImGui::DragFloat("Max Value", &sliderComponent.MaxValue, 0.1f, sliderComponent.MinValue, 100.0f);

                ImGui::Text("Handle Size");
                ImGui::DragFloat("##HandleSizeX", &sliderComponent.HandleScale.x, 0.1f, 0.1f, 5.0f);
                ImGui::DragFloat("##HandleSizeY", &sliderComponent.HandleScale.y, 0.1f, 0.1f, 5.0f);

                if (ImGui::Selectable("Background Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        sliderComponent.BackgroundTexture = texture;
                    }
                }

                if (ImGui::Selectable("Handle Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        sliderComponent.HandleTexture = texture;
                    }
                }
                if (ImGui::Selectable("Disabled Handle Texture"))
                {
                    std::string path = FileDialog::OpenFile({}).string();
                    if (!path.empty())
                    {
                        Ref<Texture2D> texture = Texture2D::Load(path);
                        sliderComponent.DisabledHandleTexture = texture;
                    }
                }
            }

            if (!isCollapsingHeaderOpen)
            {
                entity.RemoveComponent<UISliderComponent>();
            }
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

            std::string items[] = { "Tag Component", "Transform Component", "Mesh Component", "Material Component", "Light Component", "World Environment Component", "Camera Component", "Audio Source Component", "Audio Listener Component", "Audio Zone Component", "Lua Script Component", "Rigidbody Component", "Particles System Component", "NavMesh Component", "Navigation Agent Component", "Sprite Component", "UI Empty Component","UI Image Component", "UI Text Component", "UI Toggle Component", "UI Button Component", "UI Slider Component" };

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
                    if (!entity.HasComponent<MaterialComponent>())
                    {
                        entity.AddComponent<MaterialComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Light Component")
                {
                    if (!entity.HasComponent<LightComponent>())
                        entity.AddComponent<LightComponent>();
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "World Environment Component") {
                    if (!entity.HasComponent<WorldEnvironmentComponent>())
                        entity.AddComponent<WorldEnvironmentComponent>();
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
                        ImGui::CloseCurrentPopup();
                    }
                }
                else if (items[item_current] == "Rigidbody Component")
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

                            if (entity.HasComponent<TransformComponent>()) {
                                auto& transform = entity.GetComponent<TransformComponent>();
                                rbComponent.rb->SetPosition(transform.GetLocalPosition());
                                rbComponent.rb->SetRotation(transform.GetLocalRotation());
                            }

                            m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                            
                            // Set user pointer for collision detection
                            rbComponent.rb->GetNativeBody()->setUserPointer(reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));

                            // Try to automatically size the collider to the mesh AABB
                            ResizeColliderToFitMeshAABB(entity, rbComponent);
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
                else if (items[item_current] == "NavMesh Component")
                {
                    if(!entity.HasComponent<NavMeshComponent>() && entity.HasComponent<MeshComponent>() && entity.HasComponent<TransformComponent>())
                    {
                        auto& navMeshComponent = entity.AddComponent<NavMeshComponent>();
                        navMeshComponent.SetNavMesh(CreateRef<NavMesh>());
                        navMeshComponent.SetNavMeshUUID(UUID());
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Navigation Agent Component")
                {
                    if(!entity.HasComponent<NavigationAgentComponent>())
                    {
                        auto& navigationAgentComponent = entity.AddComponent<NavigationAgentComponent>();
                        navigationAgentComponent.SetPathFinder(CreateRef<NavMeshPathfinding>(nullptr));
                    }

                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Empty Component")
                {
                    if (!entity.HasComponent<UIComponent>())
                    {
                        entity.AddComponent<UIComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Image Component")
                {
                    if (!entity.HasComponent<UIImageComponent>())
                    {
                        entity.AddComponent<UIImageComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Text Component")
                {
                    if (!entity.HasComponent<UITextComponent>())
                    {
                        entity.AddComponent<UITextComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Toggle Component")
                {
                    if (!entity.HasComponent<UIToggleComponent>())
                    {
                        entity.AddComponent<UIToggleComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Button Component")
                {
                    if (!entity.HasComponent<UIButtonComponent>())
                    {
                        entity.AddComponent<UIButtonComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "UI Slider Component")
                {
                    if (!entity.HasComponent<UISliderComponent>())
                    {
                        entity.AddComponent<UISliderComponent>();
                    }
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Sprite Component")
                {
                    if (!entity.HasComponent<SpriteComponent>())
                    {
                        entity.AddComponent<SpriteComponent>();
                    }
                }
                else
                {
                    ImGui::CloseCurrentPopup();
                }
            }

            ImGui::EndPopup();
        }

/*         if (ShowMaterialOptions)
        {
            ImGui::OpenPopup("Select Material Type");
            ShowMaterialOptions = false;
        }

        if (ImGui::BeginPopupModal("Select Material Type"))
        {
            ImGui::Text("Choose the type of material to create:");
            ImGui::Separator();

            if (ImGui::Button("PBRMaterial", ImVec2(120, 0)))
            {
                entity.AddComponent<MaterialComponent>(PBRMaterial::Create("Default PBR Material"));
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();

            if (ImGui::Button("ShaderMaterial", ImVec2(120, 0)))
            {
                entity.AddComponent<MaterialComponent>(ShaderMaterial::Create("Default Shader Material"));
                ImGui::CloseCurrentPopup();
            }

            ImGui::Separator();

            if (ImGui::Button("Cancel", ImVec2(240, 0)))
            {
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        } */

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

            std::string items[] = {"Empty", "Camera", "Primitive", "Light", "World Environment", "Particle System", "Sprite2D"};
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
                    e.AddComponent<MaterialComponent>(PBRMaterial::Create("Default Material"));
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
                else if (items[item_current] == "World Environment")
                {
                    Entity e = m_Context->CreateEntity("World Environment");
                    e.AddComponent<WorldEnvironmentComponent>();
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Particle System")
                {
                    Entity e = m_Context->CreateEntity("ParticleSystem");
                    e.AddComponent<ParticlesSystemComponent>();
                    //e.AddComponent<MaterialComponent>(Material::Create("Default Particle Material"));
                    SetSelectedEntity(e);
                    ImGui::CloseCurrentPopup();
                }
                else if (items[item_current] == "Sprite2D")
                {
                    Entity e = m_Context->CreateEntity("Sprite2D");
                    e.AddComponent<SpriteComponent>();
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

    bool SceneTreePanel::ResizeColliderToFitMeshAABB(Entity entity, RigidbodyComponent& rbComponent)
    {
        // Check if entity has a mesh component
        if (entity.HasComponent<MeshComponent>()) {
            auto& meshComponent = entity.GetComponent<MeshComponent>();
            Ref<Collider> currentCollider = rbComponent.rb->GetCollider();

            // Make sure we have both a valid mesh and collider
            if (meshComponent.GetMesh() && currentCollider) {
                // Get the mesh's AABB
                const AABB& meshAABB = meshComponent.GetMesh()->GetAABB();

                // Store current rigidbody properties
                RigidBody::Properties props = rbComponent.rb->GetProperties();
                glm::vec3 position = rbComponent.rb->GetPosition();
                glm::vec3 rotation = rbComponent.rb->GetRotation();
                glm::vec3 velocity = rbComponent.rb->GetVelocity();

                // Remove from physics world
                m_Context->m_PhysicsWorld.removeRigidBody(rbComponent.rb->GetNativeBody());

                // Resize the collider to fit the mesh AABB
                rbComponent.rb->ResizeColliderToFitAABB(meshAABB);

                // Add back to physics world
                m_Context->m_PhysicsWorld.addRigidBody(rbComponent.rb->GetNativeBody());
                rbComponent.rb->GetNativeBody()->setUserPointer(
                    reinterpret_cast<void*>(static_cast<uintptr_t>((entt::entity)entity)));

                return true;
            }
        }

        return false;
    }

    void SceneTreePanel::CreatePrefab(Entity entity)
    {
        if (!entity)
            return;

        FileDialogArgs args;
        args.Filters = {{"Prefab", "prefab"}};
        args.DefaultName = entity.GetComponent<TagComponent>().Tag + ".prefab";
        const std::filesystem::path& path = FileDialog::SaveFile(args);

        if (path.empty())
            return;

        const Ref<Prefab> prefab = Prefab::Create(entity);
        prefab->Save(path);

        COFFEE_CORE_INFO("Created prefab: {0}", path.string());
    }
} // namespace Coffee