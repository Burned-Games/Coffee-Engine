#pragma once

#include "CoffeeEngine/Core/ControllerCodes.h"

#include <SDL3/SDL_gamepad.h>

namespace Coffee {

    /**
     * Wrapper for SDL controllers
     */
    class Gamepad {
    public:
        explicit Gamepad(ControllerCode id);
        ~Gamepad();

        const char* GetName() const;
        ControllerCode GetId() const;
        SDL_Gamepad* GetGamepad() const;
    private:
        SDL_Gamepad* m_gamepad;
        ControllerCode m_id;
    };

} // Coffee

