#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "IScriptingBackend.h"
#include <filesystem>
#include <unordered_map>

namespace Coffee {

    enum class ScriptingLanguage {
        Lua,
        cSharp
    };

    class ScriptManager {
    public:
        static void RegisterBackend(ScriptingLanguage language, Ref<IScriptingBackend> backend);
        static const IScriptingBackend& GetBackend(ScriptingLanguage language);
        static void RemoveBackend(ScriptingLanguage language);

        static Ref<Script> CreateScript(const std::filesystem::path& path, ScriptingLanguage language) {
            return backends[language]->CreateScript(path);
        }

        static void ExecuteScript(Script& script, ScriptingLanguage language) {
            backends[language]->ExecuteScript(script);
        }

        static void SetWorkingDirectory(const std::filesystem::path& path) {
            for (auto& backend : backends) {
                backend.second->SetWorkingDirectory(path);
            }
        }

    private:
        static std::unordered_map<ScriptingLanguage, Ref<IScriptingBackend>> backends;
    };

} // namespace Coffee