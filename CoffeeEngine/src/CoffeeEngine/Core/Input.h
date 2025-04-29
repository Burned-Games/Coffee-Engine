#pragma once

#include "CoffeeEngine/Core/ControllerCodes.h"
#include "CoffeeEngine/Core/KeyCodes.h"
#include "CoffeeEngine/Core/MouseCodes.h"
#include "CoffeeEngine/Events/ControllerEvent.h"
#include "CoffeeEngine/Events/KeyEvent.h"
#include "CoffeeEngine/Events/MouseEvent.h"
#include "CoffeeEngine/Input/Gamepad.h"
#include "CoffeeEngine/Input/InputBinding.h"
#include "Timer.h"

#include "CoffeeEngine/Events/Event.h"

#include <SDL3/SDL_gamepad.h>
#include <glm/glm.hpp>
#include <unordered_map>

namespace Coffee {

    using InputAction = uint16_t;
    /**
     * @brief List of possible actions in ActionMap v0.1
     */
    namespace ActionsEnum
    {
        enum : InputAction
        {
            // UI
            UiMoveHorizontal,
            UiMoveVertical,
            Confirm,
            Cancel,

            // Gameplay
            MoveHorizontal,
            MoveVertical,
            AimHorizontal,
            AimVertical,
            Shoot,
            Melee,
            Interact,
            Dash,
            Cover,
            Skill1,
            Skill2,
            Skill3,
            Injector,
            Grenade,
            Map,
            Pause,

            // Action count for array creation and iteration
            ActionCount
        };
    } // namespace ActionsEnum

    /**
     * @brief Possible states for rebinding inputs
     */
    enum class RebindState
    {
        None,
        PosButton,
        NegButton,
        PosKey,
        NegKey,
        Axis
    };

    /**
     * @defgroup core Core
     * @brief Core components of the CoffeeEngine.
     * @{
     */

	class Input
	{
	public:

        /**
         * Initializes the module
         */
        static void Init();

        static void Save();

        static void Load();

        /**
         * Checks if a specific key is currently being pressed.
         *
         * @param key The key code of the key to check.
         * @return True if the key is currently being pressed, false otherwise.
         */
        static bool IsKeyPressed(const KeyCode key);

        /**
         * Checks if a mouse button is currently pressed.
         *
         * @param button The mouse button to check.
         * @return True if the mouse button is pressed, false otherwise.
         */
        static bool IsMouseButtonPressed(const MouseCode button);

        /**
            * Sets the mouse cursor to be grabbed or ungrabbed.
            * When grabbed, the mouse cursor is confined to the window and hidden.
            * When ungrabbed, the mouse cursor is free to move outside the window.
            * @param grabbed True to grab the mouse cursor, false to ungrab it.
            */
        static void SetMouseGrabbed(bool grabbed);

        /**
         * Retrieves the current position of the mouse.
         *
         * @return The current position of the mouse as a 2D vector.
         */
        static const glm::vec2& GetMousePosition();
        /**
         * @brief Retrieves the current x-coordinate of the mouse cursor.
         *
         * @return The x-coordinate of the mouse cursor.
         */
        static const float GetMouseX();
        /**
         * @brief Retrieves the current y-coordinate of the mouse cursor.
         *
         * @return The y-coordinate of the mouse cursor.
         */

        static const float GetMouseY();

        static glm::vec2 GetMouseDelta();
        /**
         * @brief Checks if a specific button is currently pressed on a given controller.
         *
         * @param button The button code to check.
         * @return True if the button is pressed, false otherwise.
         */
        static bool GetButtonRaw(ButtonCode button);
        /**
         * @brief Retrieves the current value of an axis on a given controller.
         *
         * @param axis The axis code to check.
         * @return The axis value, usually between -1 and 1. Returns 0 if the controller is invalid.
         */
        static float GetAxisRaw(AxisCode axis);

        /**
         * Gets the InputBinding object for the given action
         * @param actionName The action to retrieve an InputBinding for
         * @return The InputBinding containing the bounds keys, buttons and axis for the provided action
         */
        static InputBinding& GetBinding(const std::string& actionName);

        static std::unordered_map<std::string, InputBinding>& GetAllBindings();

        /**
         *
         * @param lowFreqPower Strength of the left (low frequency) motor
         * @param highFreqPower Strength of the right (high frequency) motor
         * @param duration Vibration duration
         */
        static void SendRumble(uint16_t lowFreqPower, uint16_t highFreqPower, uint32_t duration);

        static const char* GetKeyLabel(KeyCode key);
        static const char* GetMouseButtonLabel(MouseCode button);
        static const char* GetButtonLabel(ButtonCode button);
        static const char* GetAxisLabel(AxisCode axis);

        static void StartRebindMode(std::string actionName, RebindState type);
        static void ResetRebindState();

        static void OnEvent(Event& e);

      private:

        static void GenerateDefaultMappingFile();

        /**
	     * @brief Handles controller connection events
	     * @param cEvent The event data to process
	     */
        static void OnAddController(const ControllerAddEvent* cEvent);
	    /**
         * @brief Handles controller disconnection events
         * @param cEvent The event data to process
         */
	    static void OnRemoveController(const ControllerRemoveEvent* cEvent);
	    /**
        * @brief Handles button press events from controllers.
        *
        * @param e The button press event to process.
        */
        static void OnButtonPressed(const ButtonPressEvent& e);
	    /**
         * @brief Handles button release events from controllers.
         *
         * @param e The button release event to process.
         */
        static void OnButtonReleased(const ButtonReleaseEvent& e);
	    /**
         * @brief Handles axis movement events from controllers.
         *
         * @param e The axis move event to process.
         */
        static void OnAxisMoved(const AxisMoveEvent& e);
	    /**
         * @brief Handles key press events from the keyboard.
         *
         * @param event The key pressed event to process.
         */
	    static void OnKeyPressed(const KeyPressedEvent& event);
	    /**
         * @brief Handles key release events from the keyboard.
         *
         * @param event The key released event to process.
         */
	    static void OnKeyReleased(const KeyReleasedEvent& event);
	    /**
         * @brief Handles mouse button press events.
         *
         * @param event The mouse button pressed event to process.
         */
	    static void OnMouseButtonPressed(const MouseButtonPressedEvent& event);
	    /**
         * @brief Handles mouse button release events.
         *
         * @param event The mouse button released event to process.
         */
	    static void OnMouseButtonReleased(const MouseButtonReleasedEvent& event);
	    /**
         * @brief Handles mouse movement events.
         *
         * @param event The mouse moved event to process.
         */
	    static void OnMouseMoved(const MouseMovedEvent& event);

        static std::unordered_map<std::string, InputBinding> m_BindingsMap;

	    static std::vector<Ref<Gamepad>> m_Gamepads;
	    static std::unordered_map<ButtonCode, uint8_t> m_ButtonStates;
	    static std::unordered_map<AxisCode, float> m_AxisStates;
	    static std::unordered_map<KeyCode, bool> m_KeyStates;
        static std::unordered_map<MouseCode, bool> m_MouseStates;
        static std::unordered_map<AxisCode, float> m_AxisDeadzones;
        static glm::vec2 m_MousePosition; // Position relative to window


        // Rebind mode
        static Timer m_RebindTimer;
        static RebindState m_RebindState;
	    static std::string m_RebindActionName;

    };
    /** @} */
} // namespace Coffee