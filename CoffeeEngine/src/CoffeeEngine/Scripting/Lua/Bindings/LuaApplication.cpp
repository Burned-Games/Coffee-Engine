#include "LuaApplication.h"
#include "CoffeeEngine/Core/Application.h"

void Coffee::RegisterApplicationBindings(sol::state& luaState)
{
    sol::table appTable = luaState.create_table();

    appTable.set_function("quit", []() {
        Application::Get().Close();
    });

    appTable.set_function("get_fps", []() {
        return Application::Get().GetFPS();
    });

    appTable.set_function("get_window_width", []() {
            return Application::Get().GetWindow().GetWidth();
        });

    appTable.set_function("get_window_height", []() {
        return Application::Get().GetWindow().GetHeight();
    });

    appTable.set_function("get_window_size", []() {
        auto& window = Application::Get().GetWindow();
        return std::make_pair(window.GetWidth(), window.GetHeight());
    });

    appTable.set_function("set_window_title", [](const std::string& title) {
        Application::Get().GetWindow().SetTitle(title);
    });

    appTable.set_function("get_window_title", []() {
        return Application::Get().GetWindow().GetTitle();
    });

    appTable.set_function("set_window_mode", [](WindowMode mode) {
        Application::Get().GetWindow().SetWindowMode(mode);
    });

    appTable.set_function("get_window_mode", []() {
        return Application::Get().GetWindow().GetWindowMode();
    });

    luaState.new_enum<WindowMode>("WindowMode",
        {
            {"Windowed", WindowMode::Windowed},
            {"Fullscreen", WindowMode::Fullscreen}
        }
    );

    luaState["App"] = appTable;
}