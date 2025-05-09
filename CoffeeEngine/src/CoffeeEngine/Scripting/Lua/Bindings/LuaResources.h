#pragma once
#include <sol/sol.hpp>

namespace Coffee {
    void RegisterResourcesBindings(sol::state& luaState);
    void RegisterResourceLoadingBindings(sol::state& luaState);
}