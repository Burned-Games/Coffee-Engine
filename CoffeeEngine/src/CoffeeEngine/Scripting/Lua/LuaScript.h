#pragma once

#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scripting/Lua/LuaBackend.h"
#include "CoffeeEngine/Scripting/Script.h"
#include "CoffeeEngine/Scripting/ScriptManager.h"
#include <regex>
#include <sol/forward.hpp>
#include <sol/sol.hpp>
#include <fstream>
#include <sol/types.hpp>

namespace Coffee {

    class LuaScript : public Script
    {
    public:
        LuaScript(const std::filesystem::path& path) : Script(path)
        {
            //TODO: Think if this is a good way or store it in another way is better
            const LuaBackend& backend = static_cast<const LuaBackend&>(ScriptManager::GetBackend(ScriptingLanguage::Lua));
            m_Environment = sol::environment(backend.GetLuaState(), sol::create, backend.GetLuaState().globals());
        }
        ~LuaScript() = default;

        void OnReady() override
        {
            ScriptManager::ExecuteScript(*this, ScriptingLanguage::Lua);
            const sol::protected_function& onReady = m_Environment["on_ready"];
            if (!onReady.valid()) {
                COFFEE_CORE_ERROR("Lua: on_ready function is not valid.");
                return;
            }

            const sol::protected_function_result& result = onReady();
            if (!result.valid()) {
                sol::error err = result;
                COFFEE_CORE_ERROR("Lua: {0}", err.what());
            }

            ParseScript();
        }

        void OnUpdate(float dt) override
        {
            const sol::protected_function& onUpdate = m_Environment["on_update"];
            if (!onUpdate.valid()) {
                COFFEE_CORE_ERROR("Lua: on_update function is not valid. {0}", m_Path.string());
                return;
            }

            const sol::protected_function_result& result = onUpdate(dt);
            if (!result.valid()) {
                sol::error err = result;
                COFFEE_CORE_ERROR("Lua: {0}", err.what());
            }
        }

        void OnExit() override
        {
            const sol::protected_function& onExit = m_Environment["on_exit"];
            if (!onExit.valid()) {
                COFFEE_CORE_ERROR("Lua: on_exit function is not valid.");
                return;
            }

            const sol::protected_function_result& result = onExit();
            if (!result.valid()) {
                sol::error err = result;
                COFFEE_CORE_ERROR("Lua: {0}", err.what());
            }
        }

        template<typename Ret, typename... Args>
        void RegisterFunction(const std::string& name, std::function<Ret(Args...)> function)
        {
            m_Environment[name] = function;
        }

        void CallFunction(const std::string& functionName) override
        {
            const sol::protected_function& function = m_Environment[functionName];
            if (!function.valid()) {
                COFFEE_CORE_ERROR("Lua: Function {0} is not valid.", functionName);
                return;
            }

            const sol::protected_function_result& result = function();
            if (!result.valid()) {
                sol::error err = result;
                COFFEE_CORE_ERROR("Lua: {0}", err.what());
            }
        }

        template<typename T>
        void SetVariable(const std::string& name, T value)
        {
            m_Environment[name] = value;
        }

        template <typename T>
        T GetVariable(const std::string& name)
        {
            return m_Environment[name];
        }

        void ParseScript() override
        {
            std::ifstream scriptFile(m_Path);
            std::string scriptContent((std::istreambuf_iterator<char>(scriptFile)), std::istreambuf_iterator<char>());

            // TODO: Add support for local variables

            std::regex exportRegex(R"(--\[\[export\]\]\s+(\w+)\s*=\s*(.+))"); // --[[export]] variable = value
            std::regex headerRegex(R"(--\s*\[\[header\]\]\s*(.+))"); // --[[header]] Esto es un header
            std::regex combinedRegex(R"(--\[\[export\]\]\s+(\w+)\s*=\s*(.+)|--\s*\[\[header\]\]\s*(.+))");
            std::smatch match;
            std::string::const_iterator searchStart(scriptContent.cbegin());

            while (std::regex_search(searchStart, scriptContent.cend(), match, combinedRegex)) {
                ExportedVariable variable;
                if (match[1].matched) {
                    variable.name = match[1];
                    AssignSol2VariableToStdAny(m_Environment[variable.name], variable.value);
                    variable.type = Sol2VariableToExportedVariableType(m_Environment[variable.name]);
                } else if (match[3].matched) {
                    variable.name = "header";
                    variable.value = match[3];
                    variable.type = ExportedVariableType::None;
                }

                // Store the variable in the vector
                m_ExportedVariables[variable.name] = variable;

                // Move to the next match
                searchStart = match.suffix().first;
            }
        }

        //const std::filesystem::path& GetPath() const { return m_Path; }
        const sol::environment& GetEnvironment() const { return m_Environment; }
    private:

        void AssignSol2VariableToStdAny(sol::object object, std::any& value)
        {
            switch (object.get_type())
            {
            case sol::type::boolean:
                value = object.as<bool>();
                break;
            case sol::type::number:
                value = object.as<float>();
                break;
            case sol::type::string:
                value = object.as<std::string>();
                break;
            case sol::type::userdata:
                if(object.is<Entity>())
                {
                    value = object.as<Entity>();
                }
                else if (object.is<glm::vec2>())
                {
                    value = object.as<glm::vec2>();
                }
                else if (object.is<glm::vec3>())
                {
                    value = object.as<glm::vec3>();
                }
                else if (object.is<glm::vec4>())
                {
                    value = object.as<glm::vec4>();
                }
                else {
                    value = nullptr;
                }
                break;
            case sol::type::nil:
                value = nullptr;
                break;
            default:
                value = nullptr;
                break;
            }
        }

        ExportedVariableType Sol2VariableToExportedVariableType(sol::object object)
        {
            switch (object.get_type())
            {
            case sol::type::boolean:
                return ExportedVariableType::Bool;
            case sol::type::number:
                return ExportedVariableType::Float;
            case sol::type::string:
                return ExportedVariableType::String;
            case sol::type::userdata:
                if (object.is<Entity>()) {
                    return ExportedVariableType::Entity;
                }
                else if (object.is<glm::vec2>())
                {
                    return ExportedVariableType::Vector2;
                }
                else if (object.is<glm::vec3>())
                {
                    return ExportedVariableType::Vector3;
                }
                else if (object.is<glm::vec4>())
                {
                    return ExportedVariableType::Vector4;
                }
                else {
                    return ExportedVariableType::None;
                }
            default:
                return ExportedVariableType::None;
            }
        }
    private:
        sol::environment m_Environment;
    };

}