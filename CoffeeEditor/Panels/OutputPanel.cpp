#include "OutputPanel.h"
#include "CoffeeEngine/Core/Application.h"
#include "CoffeeEngine/Core/Log.h"
#include <cstdlib>
#include <imgui.h>
#include <spdlog/spdlog.h>
#include <string>
#include <vector>
#include <regex>

namespace Coffee {

    // Helper function to remove milliseconds from timestamp
    std::string SimplifyTimestamp(const std::string& log) {
        // Regular expression to match timestamps with milliseconds: [YYYY-MM-DD HH:MM:SS.mmm]
        static std::regex timestamp_regex(R"(\[(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2})\.\d{3}\])");
        
        // Replace with timestamp without milliseconds: [YYYY-MM-DD HH:MM:SS]
        return std::regex_replace(log, timestamp_regex, "[$1]");
    }

    void OutputPanel::OnImGuiRender()
    {
        if (!m_Visible) return;

        ImGui::Begin("Output", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
        
        // Add buttons and filter
        if (ImGui::Button("Options"))
            ImGui::OpenPopup("Options");
        ImGui::SameLine();
        bool clear = ImGui::Button("Clear");
        ImGui::SameLine();
        bool copy = ImGui::Button("Copy");
        ImGui::SameLine();
        m_Filter.Draw("Filter", -100.0f);

        // Options menu
        if (ImGui::BeginPopup("Options"))
        {
            ImGui::Checkbox("Auto-scroll", &m_AutoScroll);
            ImGui::EndPopup();
        }

        ImGui::Separator();
        ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        const std::vector<std::string>& logBuffer = Coffee::Log::GetLogBuffer();
        
        // Handle clear button
        if (clear)
            Coffee::Log::ClearLogBuffer();

        // Handle copy button
        if (copy)
        {
            ImGui::LogToClipboard();
            for (const std::string& log : logBuffer)
            {
                if (m_Filter.IsActive() && !m_Filter.PassFilter(log.c_str()))
                    continue;
                
                // Use simplified timestamp for copying
                std::string simplifiedLog = SimplifyTimestamp(log);
                ImGui::LogText("%s\n", simplifiedLog.c_str());
            }
            ImGui::LogFinish();
        }

        // Display logs with filtering
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        for (const std::string& log : logBuffer)
        {
            // Skip this log if it doesn't pass the filter
            if (m_Filter.IsActive() && !m_Filter.PassFilter(log.c_str()))
                continue;

            // Simplify timestamp for display
            std::string simplifiedLog = SimplifyTimestamp(log);
            
            auto [before_level, level_str, after_level] = ParseLogMessage(simplifiedLog);
            spdlog::level::level_enum level = spdlog::level::from_str(level_str);
            
            ImGui::PushStyleColor(ImGuiCol_Text, GetLogLevelColor(level));
            ImGui::TextUnformatted(simplifiedLog.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::PopStyleVar();

        // Auto-scroll if enabled
        if (m_AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            ImGui::SetScrollHereY(1.0f);

        ImGui::EndChild();
        ImGui::End();
    }

    std::tuple<std::string, std::string, std::string> OutputPanel::ParseLogMessage(const std::string& log)
    {
        auto level_start = log.find_last_of('[');
        auto level_end = log.find(']', level_start) + 1;
        std::string level_str = log.substr(level_start + 1, level_end - level_start - 2);

        std::string before_level = log.substr(0, level_start);
        std::string after_level = log.substr(level_end);

        return {before_level, level_str, after_level};
    }

    void OutputPanel::RenderLogMessage(const std::string& before_level, const std::string& level_str, const std::string& after_level, spdlog::level::level_enum level)
    {
        ImGui::TextUnformatted(before_level.c_str());
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted("[");
        ImGui::SameLine(0.0f, 0.0f);

        ImVec4 color = GetLogLevelColor(level);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::TextUnformatted(level_str.c_str());
        ImGui::PopStyleColor();
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted("]");
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::TextUnformatted(after_level.c_str());
    }

    ImVec4 OutputPanel::GetLogLevelColor(spdlog::level::level_enum level)
    {
        switch (level)
        {
            case spdlog::level::trace: return ImVec4(0.655f, 0.596f, 0.514f, 1.0f); // #A79883
            case spdlog::level::debug: return ImGui::GetStyle().Colors[ImGuiCol_Text]; // Default text color
            case spdlog::level::info: return ImVec4(0.592f, 0.588f, 0.098f, 1.0f);  // #979619
            case spdlog::level::warn: return ImVec4(0.976f, 0.737f, 0.180f, 1.0f); // #F9BC2E
            case spdlog::level::err: return ImVec4(0.984f, 0.357f, 0.282f, 1.0f);  // #FB5B48
            case spdlog::level::critical: return ImVec4(0.796f, 0.137f, 0.110f, 1.0f); // #CB231C
            default: return ImGui::GetStyle().Colors[ImGuiCol_Text]; // Default text color
        }
    }

} // namespace Coffee