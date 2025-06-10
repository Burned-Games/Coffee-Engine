#include "GameSaver.h"
#include <iostream>

namespace Coffee {

    GameSaver& GameSaver::GetInstance()
    {
        static GameSaver instance;
        return instance;
    }

    void GameSaver::SaveVariable(const std::string& key, const SaveValue& value)
    {
        savedData[key] = value;
    }

    GameSaver::SaveValue GameSaver::LoadVariable(const std::string& key, const SaveValue& defaultValue)
    {
        if (savedData.contains(key))
            return savedData[key];

        return defaultValue;
    }

    std::string GameSaver::GetSaveFilePath()
    {
        char* prefPath = SDL_GetPrefPath("CoffeeEngine", "GameProgress");
        if (!prefPath)
        {
            std::cerr << "Error getting save path: " << SDL_GetError() << std::endl;
            return "savegame.json";
        }
        std::string filePath = std::string(prefPath) + "savegame.json";
        SDL_free(prefPath);
        return filePath;
    }

    void GameSaver::SaveToFile()
    {
        const std::string filePath = GetSaveFilePath();
        if (std::ofstream file(filePath); file)
        {
            cereal::JSONOutputArchive archive(file);
            archive(*this);
        }
        else
        {
            std::cerr << "Error saving progress to " << filePath << std::endl;
        }
    }

    void GameSaver::LoadFromFile()
    {
        const std::string filePath = GetSaveFilePath();
        if (std::ifstream file(filePath); file)
        {
            cereal::JSONInputArchive archive(file);
            archive(*this);
        }
        else
        {
            std::cerr << "No save file found, initializing default values." << std::endl;
        }
    }
}
