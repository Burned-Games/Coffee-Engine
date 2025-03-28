#pragma once

#include "CoffeeEngine/Core/Base.h"
#include "CoffeeEngine/Scripting/Script.h"
#include <filesystem>

namespace Coffee {

    class IScriptingBackend {

        public:
            /**
             * @brief Virtual destructor for the IScriptingBackend interface.
             */
            virtual ~IScriptingBackend() = default;

            /**
             * @brief Initializes the scripting backend.
             */
            virtual void Initialize() = 0;

            virtual Ref<Script> CreateScript(const std::filesystem::path& path) = 0;

            virtual void ExecuteScript(Script& script) = 0;

            virtual void SetWorkingDirectory(const std::filesystem::path& path) = 0;

            virtual void Shutdown() = 0;
    };

} // namespace Coffee