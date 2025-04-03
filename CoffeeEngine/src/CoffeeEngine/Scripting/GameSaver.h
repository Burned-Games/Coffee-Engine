#pragma once

#include <SDL3/SDL.h>
#include <cereal/archives/json.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/variant.hpp>
#include <fstream>
#include <unordered_map>
#include <variant>

namespace Coffee {

    class GameSaver {
    public:
        using SaveValue = std::variant<int, float, bool>;

        static GameSaver& GetInstance();

        void SaveVariable(const std::string& key, const SaveValue& value);
        SaveValue LoadVariable(const std::string& key, const SaveValue& defaultValue = 0);

        void SaveToFile();
        void LoadFromFile();

    private:
        std::unordered_map<std::string, SaveValue> savedData;
        static std::string GetSaveFilePath() ;

        friend class cereal::access;
        template<class Archive>
        void serialize(Archive& archive)
        {
            archive(savedData);
        }
    };

} // namespace Coffee
