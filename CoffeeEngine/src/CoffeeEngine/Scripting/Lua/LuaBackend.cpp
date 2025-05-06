#include "LuaBackend.h"

#include "Bindings/LuaApplication.h"
#include "Bindings/LuaAudio.h"
#include "Bindings/LuaComponents.h"
#include "Bindings/LuaEntity.h"
#include "Bindings/LuaInput.h"
#include "Bindings/LuaLog.h"
#include "Bindings/LuaMath.h"
#include "Bindings/LuaPhysics.h"
#include "Bindings/LuaPrefab.h"
#include "Bindings/LuaScene.h"
#include "Bindings/LuaTimer.h"
#include "CoffeeEngine/Core/Log.h"
#include "Bindings/LuaResources.h"
#include "LuaScript.h"

#include <fstream>
#include <lua.h>
#include <regex>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sol/property.hpp>
#include <sol/types.hpp>

#define SOL_PRINT_ERRORS 1

namespace Coffee {

    void LuaBackend::Initialize() {
        luaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::package, sol::lib::coroutine);

        dafaultPackagePath = luaState["package"]["path"];

        RegisterLogBindings(luaState);
        RegisterInputBindings(luaState);
        RegisterTimerBindings(luaState);
        RegisterMathBindings(luaState);
        RegisterEntityBindings(luaState);
        RegisterComponentsBindings(luaState);
        RegisterSceneBindings(luaState);
        RegisterPhysicsBindings(luaState);
        RegisterPrefabBindings(luaState);
        RegisterApplicationBindings(luaState);
        RegisterAudioBindings(luaState);
        RegisterResourcesBindings(luaState);
        RegisterResourceLoadingBindings(luaState);
    }

    Ref<Script> LuaBackend::CreateScript(const std::filesystem::path& path) {
        return CreateRef<LuaScript>(path);
    }

    void LuaBackend::ExecuteScript(Script& script) {
        LuaScript& luaScript = static_cast<LuaScript&>(const_cast<Script&>(script));
        try {
            luaState.script_file(luaScript.GetPath().string(), luaScript.GetEnvironment());
        } catch (const sol::error& e) {
            COFFEE_CORE_ERROR("Lua: {0}", e.what());
        }
    }

    void LuaBackend::SetWorkingDirectory(const std::filesystem::path& path) {
        
        luaState["package"]["path"] = dafaultPackagePath + ";" + path.string() + "/?.lua";

    }

} // namespace Coffee