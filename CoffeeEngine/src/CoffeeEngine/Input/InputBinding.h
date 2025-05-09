#pragma once

#include "CoffeeEngine/Core/ControllerCodes.h"
#include "CoffeeEngine/Core/KeyCodes.h"

#include <cereal/cereal.hpp>
#include <string>

namespace Coffee
{
    using ButtonState = uint8_t;
    namespace ButtonStates
    {
        enum : ButtonState
        {
            IDLE,
            UP,
            DOWN,
            REPEAT
        };
    } // namespace ButtonStates

    /**
     * @defgroup core Input
     * @brief Data structure containing the bound buttons, keys and axis of an input action
     * @{
     */
    class InputBinding
    {
      public:
        // This is only used for easier identification within serialized files,debug mode or config UI (if implemented)
        std::string Name = "Undefined";

        KeyCode KeyPos = Key::Unknown;
        KeyCode KeyNeg = Key::Unknown;
        ButtonCode ButtonPos = Button::Invalid;
        ButtonCode ButtonNeg = Button::Invalid;

        AxisCode Axis = Axis::Invalid;
        bool invertedAxis = false;

        // Last time this input was updated
        long latestUpdate = 0;

        InputBinding() = default;
        ~InputBinding() = default;

        template <class Archive>
        void serialize(Archive& archive, const uint32_t version)
        {
            archive(
                //CEREAL_NVP(Name),
                CEREAL_NVP(KeyPos),
                CEREAL_NVP(KeyNeg),
                CEREAL_NVP(ButtonPos),
                CEREAL_NVP(ButtonNeg),
                CEREAL_NVP(Axis)
                );
            if (version > 2)
            {
                archive(CEREAL_NVP(invertedAxis));
            }
        }

        /**
         * @brief Retrieves an input value from the action as a controller axis. It doesn't need to be an actual axis to
         * retrieve a value
         * @param digital whether the output should be rounded to an integer or not. Useful to use like directional buttons
         * @return A value between -1 and 1
         */
        float AsAxis(bool digital) const;

        /**
         * @brief Returns the value of the action as a boolean. For axes it returns true if axisVal != 0
         * @return True if any of the bound keys or buttons are pressed, or if the axis is not at 0
         */
        bool AsBool();

        /**
         * @brief Retrieves an input value from the action as a controller button. Can be used on axis and will return
         * DOWN or REPEAT if its value surpasses the defined deadzone
         * @return The current state of the action as a button
         */
        ButtonState AsButton();

      private:

        // Chainable methods for InputBinding configuration
        InputBinding& SetName(const std::string& name) { Name = name; return *this; }
        InputBinding& SetPosKey(const KeyCode code) { KeyPos = code; return *this; }
        InputBinding& SetNegKey(const KeyCode code) { KeyNeg = code; return *this; }
        InputBinding& SetButtonPos(const ButtonCode code) { ButtonPos = code; return *this; }
        InputBinding& SetButtonNeg(const ButtonCode code) { ButtonNeg = code; return *this; }
        InputBinding& SetAxis(const AxisCode code) { Axis = code; return *this; }

        ButtonState m_State = ButtonStates::IDLE;

        friend class Input;
        friend class cereal::access;
    };
    /** @} */

} // namespace Coffee

CEREAL_CLASS_VERSION(Coffee::InputBinding, 3)
