#include "LuaAudio.h"

#include "CoffeeEngine/Audio/Audio.h"

void Coffee::RegisterAudioBindings(sol::state& luaState)
{
    luaState.set_function("set_music_volume", [](const sol::object& value) {
        float volume = std::clamp(value.as<float>(), 0.0f, 1.0f);
        Audio::SetBusVolume("Music", volume);
    });

    luaState.set_function("set_sfx_volume", [](const sol::object& value) {
        float volume = std::clamp(value.as<float>(), 0.0f, 1.0f);
        Audio::SetBusVolume("SFX", volume);
    });
};
