#pragma once

#include <AK/IBytes.h>
#include <AK/SoundEngine/Common/AkTypes.h>

#include <AK/MusicEngine/Common/AkMusicEngine.h>     // Music Engine
#include <AK/SoundEngine/Common/AkMemoryMgr.h>       // Memory Manager interface

#include <AK/SoundEngine/Common/AkSoundEngine.h>     // Sound Engine
#include <AK/SoundEngine/Common/AkStreamMgrModule.h> // Stream Manager
#include <AK/SpatialAudio/Common/AkSpatialAudio.h>   // Spatial Audio

#if AK_WWISESDK_VERSION_MAJOR < 2024
#include <AK/SoundEngine/Common/AkModule.h> // Default memory manager
#else
#include <AK/SoundEngine/Common/AkMemoryMgrModule.h> // Default memory manager
#endif

#ifndef AK_OPTIMIZED
#include <AK/Comm/AkCommunication.h> // Communication (for debug builds)
#endif

#include "CoffeeEngine/Core/Base.h"
#include "AudioZone.h"

#include <AkFilePackageLowLevelIODeferred.h> // File I/O

#include <glm/vec3.hpp>
#include <vector>
#include <string>

#include <cereal/cereal.hpp>

namespace Coffee {

    struct AudioSourceComponent;
    struct AudioListenerComponent;

    /**
     * @class Audio
     * @brief Manages audio systems including memory, sound, music, and spatial audio.
     */
    class Audio
    {
    public:

        /**
         * @brief Initializes the audio system.
         */
        static void Init();

        /**
         * @brief Shuts down the audio system.
         */
        static void Shutdown();

        /**
         * @brief Processes audio.
         */
        static void ProcessAudio();

        /**
         * @brief Plays an event.
         * @param audioSourceComponent The audio source component.
         */
        static void PlayEvent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Stops an event.
         * @param audioSourceComponent The audio source component.
         */
        static void StopEvent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Pauses an event.
         * @param audioSourceComponent The audio source component.
         */
        static void PauseEvent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Resumes an event.
         * @param audioSourceComponent The audio source component.
         */
        static void ResumeEvent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Sets a switch.
         * @param switchGroup The name of the switch group.
         * @param switchState The name of the switch state.
         * @param gameObjectID The game object ID.
         */
        static void SetSwitch(const char* switchGroup, const char* switchState, uint64_t gameObjectID);

        /**
         * @brief Sets the position of a game object.
         * @param gameObjectID The game object ID.
         * @param pos Position.
         * @param forward Forward.
         * @param up Up.
         */
        static void Set3DPosition(uint64_t gameObjectID, glm::vec3 pos, glm::vec3 forward, glm::vec3 up);

        /**
         * @brief Register a game object.
         * @param gameObjectID The game object ID.
         */
        static void RegisterGameObject(uint64_t gameObjectID);

        /**
         * @brief Unregister a game object.
         * @param gameObjectID The game object ID.
         */
        static void UnregisterGameObject(uint64_t gameObjectID);

        /**
         * @brief Unregister all game objects.
         */
        static void UnregisterAllGameObjects();

        /**
         * @brief Audio bank.
         */
        struct AudioBank
        {
            std::string name; ///< The name of the bank.
            std::vector<std::string> events; ///< The events of the bank.

            /**
             * @brief Serializes the AudioBank.
             * @tparam Archive The type of the archive.
             * @param archive The archive to serialize to.
             */
            template<class Archive> void serialize(Archive& archive, std::uint32_t const version)
            {
                archive(cereal::make_nvp("Name", name), cereal::make_nvp("Events", events));
            }
        };
        
        
        /**
         * @brief Audio banks.
         */
        static std::vector<Ref<AudioBank>> audioBanks;

        /**
         * @brief Set the volume of a game object.
         * @param gameObjectID The game object ID.
         * @param newVolume The new volume.
         */
        static void SetVolume(uint64_t gameObjectID, float newVolume);

        /**
         * @brief Audio source components.
         */
        static std::vector<AudioSourceComponent*> audioSources;

        /**
         * @brief Register an audio source component.
         * @param audioSourceComponent The audio source component.
         */
        static void RegisterAudioSourceComponent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Unregister an audio source component.
         * @param audioSourceComponent The audio source component.
         */
        static void UnregisterAudioSourceComponent(AudioSourceComponent& audioSourceComponent);

        /**
         * @brief Audio listener components.
         */
        static std::vector<AudioListenerComponent*> audioListeners;

        /**
         * @brief Register an audio listener component.
         * @param audioListenerComponent The audio listener component.
         */
        static void RegisterAudioListenerComponent(AudioListenerComponent& audioListenerComponent);

        /**
         * @brief Unregister an audio listener component.
         * @param audioListenerComponent The audio listener component.
         */
        static void UnregisterAudioListenerComponent(AudioListenerComponent& audioListenerComponent);

        /**
         * @brief Play the audio sources chosen to play on awake.
         */
        static void PlayInitialAudios();

        /**
         * @brief Stop all audio events.
         */
        static void StopAllEvents();

        /**
         * @brief Sets the volume of an audio bus.
         * @param busName The name of the bus.
         * @param volume The volume value (0.0 to 1.0).
         */
        static void SetBusVolume(const char* busName, float volume);

    private:

        /**
         * @brief Initializes the memory manager.
         * @return True if successful, false otherwise.
         */
        static bool InitializeMemoryManager();

        /**
         * @brief Initializes the stream manager.
         * @return True if successful, false otherwise.
         */
        static bool InitializeStreamManager();

        /**
         * @brief Initializes low-level I/O.
         * @return True if successful, false otherwise.
         */
        static bool InitializeLowLevelIO();

        /**
         * @brief Initializes the sound engine.
         * @return True if successful, false otherwise.
         */
        static bool InitializeSoundEngine();

        /**
         * @brief Initializes the music engine.
         * @return True if successful, false otherwise.
         */
        static bool InitializeMusicEngine();

        /**
         * @brief Initializes spatial audio.
         * @return True if successful, false otherwise.
         */
        static bool InitializeSpatialAudio();

        /**
         * @brief Initializes the communication module.
         * @return True if successful, false otherwise.
         */
        static bool InitializeCommunicationModule();

        /**
         * @brief Loads audio banks.
         * @return True if successful, false otherwise.
         */
        static bool LoadAudioBanks();
    };
} // namespace Coffee
CEREAL_CLASS_VERSION(Coffee::Audio::AudioBank, 0);
