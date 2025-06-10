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

    private:

        static void BeginHorizontalChild(const char* label, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
      void SetSelectedBinding(std::string actionName, InputBinding* binding);
      void RenderInputSettings(ImGuiWindowFlags flags);
        void RenderGeneralSettings(ImGuiWindowFlags flags);

        uint8_t m_VisiblePanels = 0;


        InputBinding* m_SelectedInputBinding = nullptr;
        std::string m_SelectedInputKey = "";

        std::array<char, 256> arr_newBindName;

    };

} // Coffee
