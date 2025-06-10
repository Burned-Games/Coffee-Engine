#include "AudioZone.h"

#include "CoffeeEngine/Scene/Components.h"

#include <AK/SoundEngine/Common/AkSoundEngine.h>
#include <AK/Plugin/AllPluginsFactories.h>

#include <unordered_map>
#include <fstream>

namespace Coffee
{
    uint64_t m_nextZoneID= 1000;

    std::unordered_map<uint64_t, AudioZoneComponent*> m_zones;

    std::unordered_map<uint64_t, glm::vec3> m_registeredObjects;

    std::vector<std::string> AudioZone::busNames;

    void AudioZone::Shutdown()
    {
        RemoveAllReverbZones();
        m_registeredObjects.clear();
    }

    void AudioZone::CreateZone(AudioZoneComponent& audioZone)
    {
        for (const auto& zone : m_zones)
        {
            if (zone.first == audioZone.zoneID)
                return;
        }

        if (audioZone.zoneID == -1)
            audioZone.zoneID = UUID();

        AK::SoundEngine::RegisterGameObj(audioZone.zoneID);

        m_zones[audioZone.zoneID] = &audioZone;
    }

    void AudioZone::UpdateReverbZone(const AudioZoneComponent& audioZoneComponent)
    {
        auto it = m_zones.find(audioZoneComponent.zoneID);
        if (it != m_zones.end())
        {
            it->second->position = audioZoneComponent.position;
            it->second->radius = audioZoneComponent.radius;
            it->second->audioBusName = audioZoneComponent.audioBusName;
        }
    }

    void AudioZone::RemoveReverbZone(const AudioZoneComponent& audioZoneComponent)
    {
        CleanupZone(audioZoneComponent.zoneID);
        m_zones.erase(audioZoneComponent.zoneID);
    }

    void AudioZone::RemoveAllReverbZones()
    {
        std::vector<uint64_t> zoneIDs;
        for (const auto& zone : m_zones)
        {
            zoneIDs.push_back(zone.first);
        }

        for (const auto& id : zoneIDs)
        {
            RemoveReverbZone(*m_zones[id]);
        }

        m_zones.clear();
    }

    void AudioZone::SetObjectInReverbZone(const uint64_t objectID, const std::vector<AudioZoneComponent*>& audioZones)
    {
        std::vector<AkAuxSendValue> auxSends;

        for (const auto& audioZone : audioZones)
        {
            AkAuxSendValue auxSend;
            auxSend.listenerID = AK_INVALID_GAME_OBJECT;
            auxSend.auxBusID = AK::SoundEngine::GetIDFromString(audioZone->audioBusName.c_str());
            auxSend.fControlValue = EFFECT_LEVEL_ENABLED;

            auxSends.push_back(auxSend);
        }

        AK::SoundEngine::SetGameObjectAuxSendValues(objectID, auxSends.data(), static_cast<AkUInt32>(auxSends.size()));
    }

    void AudioZone::RegisterObject(const uint64_t objectID, const glm::vec3& position)
    {
        m_registeredObjects[objectID] = position;
    }

    void AudioZone::UnregisterObject(const uint64_t objectID)
    {
        m_registeredObjects.erase(objectID);
    }

    void AudioZone::UpdateObjectPosition(const uint64_t objectID, const glm::vec3& position)
    {
        m_registeredObjects[objectID] = position;
    }

    void AudioZone::Update()
    {
        for (auto& object : m_registeredObjects)
        {
            std::vector<AudioZoneComponent*> zonesForObject;

            for (const auto& zone : m_zones)
            {
                if (IsObjectInZone(object.second, *zone.second))
                {
                    zonesForObject.push_back(zone.second);
                }
            }

            SetObjectInReverbZone(object.first, zonesForObject);
        }
    }

    void AudioZone::CleanupZone(const uint64_t zoneID)
    {
        AK::SoundEngine::UnregisterGameObj(zoneID);
    }

    bool AudioZone::IsObjectInZone(const glm::vec3& objectPos, const AudioZoneComponent& audioZoneComponent)
    {
        const float dx = objectPos.x - audioZoneComponent.position.x;
        const float dy = objectPos.y - audioZoneComponent.position.y;
        const float dz = objectPos.z - audioZoneComponent.position.z;
        const float distanceSquared = dx * dx + dy * dy + dz * dz;
        return distanceSquared <= (audioZoneComponent.radius * audioZoneComponent.radius);
    }

    bool AudioZone::SearchAvailableBusChannels()
    {
        std::ifstream file(std::filesystem::absolute(Audio::GetAudioPath() / "SoundbanksInfo.json"));
        if (!file.is_open())
            return false;

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        rapidjson::Document banksInfo;
        if (banksInfo.Parse(buffer.str().c_str()).HasParseError()
            || !banksInfo.HasMember("SoundBanksInfo")
            || !banksInfo["SoundBanksInfo"].HasMember("SoundBanks"))
        {
            return false;
        }

        const auto& soundBanks = banksInfo["SoundBanksInfo"]["SoundBanks"];
        if (!soundBanks.IsArray())
            return false;

        for (const auto& bankData : soundBanks.GetArray())
        {
            if (!bankData.HasMember("AuxBusses") || (bankData.HasMember("ShortName") && bankData["ShortName"].GetString() != std::string("Init")))
                continue;

            if (bankData.HasMember("AuxBusses") && bankData["AuxBusses"].IsArray())
            {
                for (const auto& busData : bankData["AuxBusses"].GetArray())
                {
                    if (busData.HasMember("Name") && busData["Name"].IsString())
                    {
                        std::string busName = busData["Name"].GetString();
                        busNames.push_back(busName);
                    }
                }
            }
        }

        return true;
    }

}
