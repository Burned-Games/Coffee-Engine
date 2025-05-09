#include "ContentBrowserPanel.h"
#include "CoffeeEngine/IO/Resource.h"
#include "CoffeeEngine/IO/ResourceLoader.h"
#include "CoffeeEngine/IO/ResourceRegistry.h"
#include "CoffeeEngine/Project/Project.h"
#include "CoffeeEngine/Renderer/Model.h"
#include "CoffeeEngine/Renderer/Texture.h"
#include "CoffeeEngine/Scene/Prefab.h"
#include "CoffeeEngine/Scene/Scene.h"
#include <CoffeeEngine/IO/ResourceUtils.h>
#include <IconsLucide.h>
#include <SDL3/SDL_misc.h>
#include <filesystem>
#include <imgui.h>
#include <string>

namespace Coffee {

    ContentBrowserPanel::ContentBrowserPanel(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void ContentBrowserPanel::SetContext(const Ref<Scene>& scene)
    {
        m_Context = scene;
    }

    void ContentBrowserPanel::OnImGuiRender()
    {
        if (!m_Visible) return;

        ImGui::Begin("Content Browser");

        if(Project::GetActive() and !Project::GetActive()->GetProjectDirectory().empty())
        {
            if(m_CurrentDirectory != Project::GetActive()->GetProjectDirectory())
            {
                m_CurrentDirectory = Project::GetActive()->GetProjectDirectory();
            }

            DisplayDirectoryContents(m_CurrentDirectory, 0);
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextWrapped("No project loaded, create or open a project to see its contents");
            ImGui::PopStyleColor();
        }

        ImGui::End();
    }

    // Function to format file size
    std::string FormatFileSize(std::uintmax_t size)
    {
        const char* units[] = { "B", "KB", "MB", "GB", "TB" };
        int unitIndex = 0;
        double displaySize = static_cast<double>(size);

        while (displaySize >= 1024 && unitIndex < 4)
        {
            displaySize /= 1024;
            ++unitIndex;
        }

        char formattedSize[20];
        snprintf(formattedSize, sizeof(formattedSize), "%.2f %s", displaySize, units[unitIndex]);
        return std::string(formattedSize);
    }

    void ContentBrowserPanel::DisplayDirectoryContents(const std::filesystem::path& directory, int depth)
    {
        for (auto& directoryEntry : std::filesystem::directory_iterator(directory))
        {
            const auto& path = directoryEntry.path();

            if(directoryEntry.is_regular_file() and GetResourceTypeFromExtension(path) == ResourceType::Unknown)
            {
                continue;
            }

            if(directoryEntry.is_directory() and directoryEntry.path().filename().string().front() == '.')
            {
                continue;
            }

            auto relativePath = std::filesystem::relative(path, m_CurrentDirectory);
            std::string filenameString = relativePath.filename().string();

            ImGuiTreeNodeFlags flags = ((m_SelectedDirectory == path) ? ImGuiTreeNodeFlags_Selected : 0) |
                                       (directoryEntry.is_directory() ? 0 : ImGuiTreeNodeFlags_Leaf) |
                                       ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanAvailWidth;

            const char* icon = directoryEntry.is_directory() ? ICON_LC_FOLDER : ICON_LC_FILE;
            if (!directoryEntry.is_directory())
            {
                ResourceType type = GetResourceTypeFromExtension(path);

                switch (type)
                {
                    using enum ResourceType;
                    
                    case Texture2D:
                        icon = ICON_LC_IMAGE;
                        break;
                    case Model:
                        icon = ICON_LC_BOX;
                        break;
                    /* case Material:
                        icon = ICON_LC_PAINT;
                        break; */
                    case Shader:
                        icon = ICON_LC_SQUARE_CHART_GANTT;
                        break;
                    case Cubemap:
                        icon = ICON_LC_IMAGES;
                        break;
                    case Prefab:
                        icon = ICON_LC_PACKAGE;
                        break;
                    /* case Scene:
                        icon = ICON_LC_CLAPPERBOARD;
                        break; */
                    case Unknown:
                        icon = ICON_LC_FILE_QUESTION;
                        break;
                    default:
                        icon = ICON_LC_FILE;
                        break;
                }
            }

            if (ImGui::TreeNodeEx((std::string(icon) + " " + filenameString).c_str(), flags))
            {
                if (ImGui::BeginDragDropSource())
                {
                    // Only load non-prefab resources immediately
                    if (GetResourceTypeFromExtension(path) != ResourceType::Prefab)
                    {
                        Ref<Resource> resource = ResourceRegistry::Get<Resource>(path.filename().string());
                        if (resource)
                        {
                            ImGui::SetDragDropPayload("RESOURCE", &resource, sizeof(Ref<Resource>));
                            ImGui::Text("%s", filenameString.c_str());
                        }
                    }
                    else
                    {
                        // For prefabs, just pass the path
                        std::filesystem::path absolutePath = std::filesystem::absolute(path);
                        ImGui::SetDragDropPayload("PREFAB_PATH", absolutePath.string().c_str(), absolutePath.string().size() + 1);
                        ImGui::Text("%s (Prefab)", filenameString.c_str());
                    }
                    
                    ImGui::EndDragDropSource();
                }

                if(ImGui::IsItemHovered())
                {
                    if(directoryEntry.is_directory())
                    {
                        ImGui::BeginTooltip();
                        ImGui::Text("%s", filenameString.c_str());
                        ImGui::Text("Type: Directory");
                        ImGui::EndTooltip();
                    }
                    else
                    {
                        Ref<Resource> resource = ResourceRegistry::Get<Resource>(path.filename().string());
                        
                        ImGui::BeginTooltip();
                        
                        if (resource)
                        {
                            try
                            {
                                ResourceType resourceType = resource->GetType();
                                
                                switch (resourceType)
                                {
                                    case ResourceType::Texture2D:
                                    {
                                        Ref<Texture2D> texture = std::static_pointer_cast<Texture2D>(resource);
                                        ImGui::Image((void*)(intptr_t)texture->GetID(), ImVec2(64, 64));
                                        ImGui::SameLine();
                                        ImGui::BeginGroup();
                                        ImGui::Text("%s", resource->GetName().c_str());
                                        ImGui::Text("Size: %s", FormatFileSize(std::filesystem::directory_entry(path).file_size()).c_str());
                                        ImGui::Text("Type: %s", ResourceTypeToString(resource->GetType()).c_str());
                                        //ImGui::Text("Image Format: %s", ImageFormatToString(texture->GetImageFormat()).c_str());
                                        ImGui::Text("Dimensions: %d x %d", texture->GetWidth(), texture->GetHeight());
                                        ImGui::EndGroup();
                                        break;
                                    }

                                    default:
                                        ImGui::Text("%s", resource->GetName().c_str());
                                        ImGui::Text("Size: %s", FormatFileSize(std::filesystem::directory_entry(path).file_size()).c_str());
                                        ImGui::Text("Type: %s", ResourceTypeToString(resourceType).c_str());
                                        break;
                                }
                            }
                            catch (...)
                            {
                                // Fallback for any exception
                                ImGui::Text("%s", path.filename().string().c_str());
                                ImGui::Text("Size: %s", FormatFileSize(std::filesystem::directory_entry(path).file_size()).c_str());
                                ImGui::Text("Type: %s", ResourceTypeToString(GetResourceTypeFromExtension(path)).c_str());
                            }
                        }
                        else
                        {
                            // Fallback for when resource isn't registered
                            ImGui::Text("%s", path.filename().string().c_str());
                            ImGui::Text("Size: %s", FormatFileSize(std::filesystem::directory_entry(path).file_size()).c_str());
                            ImGui::Text("Type: %s", ResourceTypeToString(GetResourceTypeFromExtension(path)).c_str());
                        }

                        ImGui::EndTooltip();
                    }
                }

                if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
                {
                    m_SelectedDirectory = path;
                    m_SelectedResource = directoryEntry.is_directory() ? nullptr : ResourceRegistry::Get<Resource>(path.filename().string());
                }
                if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
                {
                    // Handle right-click actions
                    // Open context menu
                }

                if (directoryEntry.is_directory())
                    DisplayDirectoryContents(path, depth + 1);

                ImGui::TreePop();
            }
            if (ImGui::BeginPopupContextItem())
            {
                // Show dependencies
                // Show owners
                // ----------------
                // Copy path
                // Copy absolute path
                // Copy UUID
                // Rename
                // Duplicate
                // Delete
                // ----------------
                // Reimport
                // ----------------
                // Show in Explorer

                if (ImGui::MenuItem("Copy Path"))
                {
                    ImGui::SetClipboardText(path.string().c_str());
                }

                if (ImGui::MenuItem("Copy Absolute Path"))
                {
                    ImGui::SetClipboardText(std::filesystem::absolute(path).string().c_str());
                }

                if (ImGui::MenuItem("Copy UUID"))
                {
                    const Ref<Resource>& resource = ResourceRegistry::Get<Resource>(path.filename().string());
                    ImGui::SetClipboardText(std::to_string(resource->GetUUID()).c_str());
                }

                if (ImGui::MenuItem("Rename", nullptr, false, false))
                {
                    // Rename the file
                }

                if (ImGui::MenuItem("Duplicate", nullptr, false, false))
                {
                    // Duplicate the file
                }

                if (ImGui::MenuItem("Delete"))
                {
                    const Ref<Resource>& resource = ResourceRegistry::Get<Resource>(path.filename().string());
                    ResourceLoader::RemoveResource(resource);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Reimport"))
                {
                    const Ref<Resource>& resource = ResourceRegistry::Get<Resource>(path.filename().string());
                    ResourceLoader::ReimportResource(resource);
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Show in Explorer"))
                {
                    // TODO: Abstract this to library independent code
                    SDL_OpenURL(("file://" + std::filesystem::absolute(path).string()).c_str());
                }

                ImGui::EndPopup();
            }
        }
    }

}
