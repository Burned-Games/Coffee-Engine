#pragma once

#include "Panel.h"

namespace Coffee {
    class InputBinding;


    namespace PanelDisplayEnum
    {
        enum : uint8_t{
            None = 0,
            General = BIT(1),
            Input = BIT(2)
        };
    }

    class ProjectSettingsPanel : public Panel
    {
    public:
        ProjectSettingsPanel() = default;

        void OnImGuiRender() override;

        bool IsPanelVisible(uint8_t panelMask) const { return m_VisiblePanels & panelMask; }

    private:

        static void BeginHorizontalChild(const char* label, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
        void SetSelectedBinding(std::string actionName, InputBinding* binding);
        void RenderInputSettings(ImGuiWindowFlags flags);
        void RenderGeneralSettings(ImGuiWindowFlags flags);

        /**
         * @brief Set what panels to display in the Project Settings window. If multiple panels are set, they will
         * display one after the other
         * @param panelMask The panels to display
         */
        void SetPanelVisible(const uint8_t panelMask){ m_VisiblePanels = panelMask; m_RefreshPanels |= panelMask; }

        uint8_t m_VisiblePanels = 0;
        uint8_t m_RefreshPanels = 0; // Used to mark panels for manual refresh when opened

        // General settings
        std::string m_NewProjectName;

        // Input settings
        InputBinding* m_SelectedInputBinding = nullptr;
        std::string m_SelectedInputKey;

    };

} // Coffee
