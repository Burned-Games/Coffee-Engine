#include "CoffeeEngine/Input/InputBinding.h"

#include "CoffeeEngine/Core/Input.h"

namespace Coffee {

    float InputBinding::AsAxis(bool digital) const
    {
        float value = Input::GetAxisRaw(Axis);
        if (invertedAxis)
            value = -value;

        value += (Input::IsKeyPressed(KeyPos) || Input::GetButtonRaw(ButtonPos));
        value -= (Input::IsKeyPressed(KeyNeg) || Input::GetButtonRaw(ButtonNeg));

        value = glm::clamp(value, -1.0f, 1.0f);

        if (digital)
            return glm::round(value);
        else
            return value;
    }
    bool InputBinding::AsBool()
    {
        bool value = glm::abs(Input::GetAxisRaw(Axis)) != 0.0f;
        value |= Input::GetButtonRaw(ButtonPos);
        value |= Input::IsKeyPressed(KeyPos);

        // "Negative" buttons and keys used as alternatives to the main button and key
        value |= Input::GetButtonRaw(ButtonNeg);
        value |= Input::IsKeyPressed(KeyNeg);

        return value;
    }

    ButtonState InputBinding::AsButton()
    {
        bool value = this->AsBool();

        // Set button state
        // Pressed button while Not Down or Repeat -> Down
        // Pressed button while Down -> Repeat
        // Release button while not Idle or Up -> Up
        // Button released while Up -> Idle
        switch (m_State)
        {
            using namespace ButtonStates;
            case IDLE: {
                if (value) m_State = DOWN;
                break;
            }
            case DOWN: {
                if (value) m_State = REPEAT;
                else m_State = UP;
                break;
            }
            case REPEAT: {
                if (!value) m_State = UP;
                break;
            }
            case UP: {
                if (value) m_State = DOWN;
                break;
            }
        }

        return m_State;
    }

} // namespace Coffee
