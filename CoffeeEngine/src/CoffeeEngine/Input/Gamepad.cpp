//
// Created by Roger on 23/2/2025.
//

#include "Gamepad.h"
#include "CoffeeEngine/Core/Log.h"

namespace Coffee {

    Gamepad::Gamepad(ControllerCode id)
    {
        m_gamepad = SDL_OpenGamepad(id);
        if (m_gamepad == nullptr)
        {
            COFFEE_ERROR("Gamepad could not be opened: %s", SDL_GetError());
        }
        else
        {
            COFFEE_INFO("Gamepad {0} initialized: {1}", id, GetName());
            m_id = id;
        }
    }

    Gamepad::~Gamepad()
    {
        if (m_gamepad)
            COFFEE_INFO("Gamepad {0} disconnected: {1}", m_id, GetName());
        SDL_CloseGamepad(m_gamepad);
    }

    const char* Gamepad::GetName() const { return SDL_GetGamepadName(m_gamepad); }

    ControllerCode Gamepad::GetId() const
    {
        return m_id;
    }
    SDL_Gamepad* Gamepad::GetGamepad() const
    {
        return m_gamepad;
    }

} // Coffee