#include "LuaLog.h"

#include "CoffeeEngine/Core/Log.h"
void Coffee::RegisterLogBindings(sol::state& luaState)
{
    luaState.set_function("log", [](const std::string& message) {
        COFFEE_CORE_INFO("{0}", message);
    });

    luaState.set_function("log_warning", [](const std::string& message) {
        COFFEE_CORE_WARN("{0}", message);
    });

    luaState.set_function("log_error", [](const std::string& message) {
        COFFEE_CORE_ERROR("{0}", message);
    });

    luaState.set_function("log_critical", [](const std::string& message) {
        COFFEE_CORE_CRITICAL("{0}", message);
    });
}