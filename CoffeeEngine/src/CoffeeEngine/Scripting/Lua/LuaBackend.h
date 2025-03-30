#pragma once
#include "CoffeeEngine/Scripting/IScriptingBackend.h"

#include <sol/sol.hpp>
#include <string>

namespace Coffee {

    class LuaScript;
    class Entity;
    struct RigidbodyComponent;

    class LuaBackend : public IScriptingBackend {
        public:
            void Initialize() override;
            
            Ref<Script> CreateScript(const std::filesystem::path& path) override;
            void ExecuteScript(Script& script) override;

            const sol::state& GetLuaState() const { return luaState; }

            void SetWorkingDirectory(const std::filesystem::path& path) override;

            void Shutdown() override {}


            bool ResizeColliderToFitMeshAABB(Coffee::Entity entity, RigidbodyComponent& rbComponent);

        private:
            sol::state luaState;
            std::string dafaultPackagePath;
    };

} // namespace Coffee