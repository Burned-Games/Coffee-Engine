#pragma once

#include <SDL3/SDL.h>
#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <fstream>
#include <unordered_map>
#include <variant>

namespace Coffee {

    /**
     * @brief Class responsible for saving and loading game variables.
     */
    class GameSaver {
    public:
        using SaveValue = std::variant<int, float, bool>; ///< Alias for the type of values that can be saved.

        /**
         * @brief Gets the singleton instance of the GameSaver.
         * @return Reference to the GameSaver instance.
         */
        static GameSaver& GetInstance();

        /**
         * @brief Saves a variable with a given key and value.
         * @param key The key associated with the variable.
         * @param value The value to save.
         */
        void SaveVariable(const std::string& key, const SaveValue& value);

        /**
         * @brief Loads a variable by its key.
         * @param key The key associated with the variable.
         * @param defaultValue The default value to return if the key is not found.
         * @return The loaded value or the default value if the key is not found.
         */
        SaveValue LoadVariable(const std::string& key, const SaveValue& defaultValue = 0);

        /**
         * @brief Saves all variables to a file.
         */
        void SaveToFile();

        /**
         * @brief Loads all variables from a file.
         */
        void LoadFromFile();

    private:
        std::unordered_map<std::string, SaveValue> savedData; ///< Map storing saved variables.

        /**
         * @brief Gets the file path for saving and loading data.
         * @return The file path as a string.
         */
        static std::string GetSaveFilePath();

        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(savedData);
        }

        friend class cereal::access;
    };

} // namespace Coffee
