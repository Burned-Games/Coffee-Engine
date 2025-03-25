#include "LuaBackend.h"

#include "CoffeeEngine/Core/ControllerCodes.h"
#include "CoffeeEngine/Core/Input.h"
#include "CoffeeEngine/Core/KeyCodes.h"
#include "CoffeeEngine/Core/Log.h"
#include "CoffeeEngine/Core/MouseCodes.h"
#include "CoffeeEngine/Core/Timer.h"
#include "CoffeeEngine/Core/Stopwatch.h"

#include "CoffeeEngine/Scene/Components.h"
#include "CoffeeEngine/Scene/Entity.h"
#include "CoffeeEngine/Scene/SceneManager.h"
#include "CoffeeEngine/Scripting/Lua/LuaScript.h"
#include <fstream>
#include <lua.h>
#include <regex>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#define SOL_PRINT_ERRORS 1

namespace Coffee {

    sol::state LuaBackend::luaState;

    void BindKeyCodesToLua(sol::state& lua, sol::table& inputTable)
    {
        std::vector<std::pair<std::string, KeyCode>> keyCodes = {
            {"Unknown", Key::Unknown},
            {"A", Key::A},
            {"B", Key::B},
            {"C", Key::C},
            {"D", Key::D},
            {"E", Key::E},
            {"F", Key::F},
            {"G", Key::G},
            {"H", Key::H},
            {"I", Key::I},
            {"J", Key::J},
            {"K", Key::K},
            {"L", Key::L},
            {"M", Key::M},
            {"N", Key::N},
            {"O", Key::O},
            {"P", Key::P},
            {"Q", Key::Q},
            {"R", Key::R},
            {"S", Key::S},
            {"T", Key::T},
            {"U", Key::U},
            {"V", Key::V},
            {"W", Key::W},
            {"X", Key::X},
            {"Y", Key::Y},
            {"Z", Key::Z},
            {"D1", Key::D1},
            {"D2", Key::D2},
            {"D3", Key::D3},
            {"D4", Key::D4},
            {"D5", Key::D5},
            {"D6", Key::D6},
            {"D7", Key::D7},
            {"D8", Key::D8},
            {"D9", Key::D9},
            {"D0", Key::D0},
            {"Return", Key::Return},
            {"Escape", Key::Escape},
            {"Backspace", Key::Backspace},
            {"Tab", Key::Tab},
            {"Space", Key::Space},
            {"Minus", Key::Minus},
            {"Equals", Key::Equals},
            {"LeftBracket", Key::LeftBracket},
            {"RightBracket", Key::RightBracket},
            {"Backslash", Key::Backslash},
            {"NonUsHash", Key::NonUsHash},
            {"Semicolon", Key::Semicolon},
            {"Apostrophe", Key::Apostrophe},
            {"Grave", Key::Grave},
            {"Comma", Key::Comma},
            {"Period", Key::Period},
            {"Slash", Key::Slash},
            {"CapsLock", Key::CapsLock},
            {"F1", Key::F1},
            {"F2", Key::F2},
            {"F3", Key::F3},
            {"F4", Key::F4},
            {"F5", Key::F5},
            {"F6", Key::F6},
            {"F7", Key::F7},
            {"F8", Key::F8},
            {"F9", Key::F9},
            {"F10", Key::F10},
            {"F11", Key::F11},
            {"F12", Key::F12},
            {"PrintScreen", Key::PrintScreen},
            {"ScrollLock", Key::ScrollLock},
            {"Pause", Key::Pause},
            {"Insert", Key::Insert},
            {"Home", Key::Home},
            {"PageUp", Key::PageUp},
            {"Delete", Key::Delete},
            {"End", Key::End},
            {"PageDown", Key::PageDown},
            {"Right", Key::Right},
            {"Left", Key::Left},
            {"Down", Key::Down},
            {"Up", Key::Up},
            {"NumLockClear", Key::NumLockClear},
            {"KpDivide", Key::KpDivide},
            {"KpMultiply", Key::KpMultiply},
            {"KpMinus", Key::KpMinus},
            {"KpPlus", Key::KpPlus},
            {"KpEnter", Key::KpEnter},
            {"Kp1", Key::Kp1},
            {"Kp2", Key::Kp2},
            {"Kp3", Key::Kp3},
            {"Kp4", Key::Kp4},
            {"Kp5", Key::Kp5},
            {"Kp6", Key::Kp6},
            {"Kp7", Key::Kp7},
            {"Kp8", Key::Kp8},
            {"Kp9", Key::Kp9},
            {"Kp0", Key::Kp0},
            {"KpPeriod", Key::KpPeriod},
            {"NonUsBackslash", Key::NonUsBackslash},
            {"Application", Key::Application},
            {"Power", Key::Power},
            {"KpEquals", Key::KpEquals},
            {"F13", Key::F13},
            {"F14", Key::F14},
            {"F15", Key::F15},
            {"F16", Key::F16},
            {"F17", Key::F17},
            {"F18", Key::F18},
            {"F19", Key::F19},
            {"F20", Key::F20},
            {"F21", Key::F21},
            {"F22", Key::F22},
            {"F23", Key::F23},
            {"F24", Key::F24},
            {"Execute", Key::Execute},
            {"Help", Key::Help},
            {"Menu", Key::Menu},
            {"Select", Key::Select},
            {"Stop", Key::Stop},
            {"Again", Key::Again},
            {"Undo", Key::Undo},
            {"Cut", Key::Cut},
            {"Copy", Key::Copy},
            {"Paste", Key::Paste},
            {"Find", Key::Find},
            {"Mute", Key::Mute},
            {"VolumeUp", Key::VolumeUp},
            {"VolumeDown", Key::VolumeDown},
            {"KpComma", Key::KpComma},
            {"KpEqualsAs400", Key::KpEqualsAs400},
            {"International1", Key::International1},
            {"International2", Key::International2},
            {"International3", Key::International3},
            {"International4", Key::International4},
            {"International5", Key::International5},
            {"International6", Key::International6},
            {"International7", Key::International7},
            {"International8", Key::International8},
            {"International9", Key::International9},
            {"Lang1", Key::Lang1},
            {"Lang2", Key::Lang2},
            {"Lang3", Key::Lang3},
            {"Lang4", Key::Lang4},
            {"Lang5", Key::Lang5},
            {"Lang6", Key::Lang6},
            {"Lang7", Key::Lang7},
            {"Lang8", Key::Lang8},
            {"Lang9", Key::Lang9},
            {"AltErase", Key::AltErase},
            {"SysReq", Key::SysReq},
            {"Cancel", Key::Cancel},
            {"Clear", Key::Clear},
            {"Prior", Key::Prior},
            {"Return2", Key::Return2},
            {"Separator", Key::Separator},
            {"Out", Key::Out},
            {"Oper", Key::Oper},
            {"ClearAgain", Key::ClearAgain},
            {"CrSel", Key::CrSel},
            {"ExSel", Key::ExSel},
            {"Kp00", Key::Kp00},
            {"Kp000", Key::Kp000},
            {"ThousandsSeparator", Key::ThousandsSeparator},
            {"DecimalSeparator", Key::DecimalSeparator},
            {"CurrencyUnit", Key::CurrencyUnit},
            {"CurrencySubUnit", Key::CurrencySubUnit},
            {"KpLeftParen", Key::KpLeftParen},
            {"KpRightParen", Key::KpRightParen},
            {"KpLeftBrace", Key::KpLeftBrace},
            {"KpRightBrace", Key::KpRightBrace},
            {"KpTab", Key::KpTab},
            {"KpBackspace", Key::KpBackspace},
            {"KpA", Key::KpA},
            {"KpB", Key::KpB},
            {"KpC", Key::KpC},
            {"KpD", Key::KpD},
            {"KpE", Key::KpE},
            {"KpF", Key::KpF},
            {"KpXor", Key::KpXor},
            {"KpPower", Key::KpPower},
            {"KpPercent", Key::KpPercent},
            {"KpLess", Key::KpLess},
            {"KpGreater", Key::KpGreater},
            {"KpAmpersand", Key::KpAmpersand},
            {"KpDblAmpersand", Key::KpDblAmpersand},
            {"KpVerticalBar", Key::KpVerticalBar},
            {"KpDblVerticalBar", Key::KpDblVerticalBar},
            {"KpColon", Key::KpColon},
            {"KpHash", Key::KpHash},
            {"KpSpace", Key::KpSpace},
            {"KpAt", Key::KpAt},
            {"KpExclam", Key::KpExclam},
            {"KpMemStore", Key::KpMemStore},
            {"KpMemRecall", Key::KpMemRecall},
            {"KpMemClear", Key::KpMemClear},
            {"KpMemAdd", Key::KpMemAdd},
            {"KpMemSubtract", Key::KpMemSubtract},
            {"KpMemMultiply", Key::KpMemMultiply},
            {"KpMemDivide", Key::KpMemDivide},
            {"KpPlusMinus", Key::KpPlusMinus},
            {"KpClear", Key::KpClear},
            {"KpClearEntry", Key::KpClearEntry},
            {"KpBinary", Key::KpBinary},
            {"KpOctal", Key::KpOctal},
            {"KpDecimal", Key::KpDecimal},
            {"KpHexadecimal", Key::KpHexadecimal},
            {"LCtrl", Key::LCtrl},
            {"LShift", Key::LShift},
            {"LAlt", Key::LAlt},
            {"LGui", Key::LGui},
            {"RCtrl", Key::RCtrl},
            {"RShift", Key::RShift},
            {"RAlt", Key::RAlt},
            {"RGui", Key::RGui},
            {"Mode", Key::Mode},
            {"Sleep", Key::Sleep},
            {"Wake", Key::Wake},
            {"ChannelIncrement", Key::ChannelIncrement},
            {"ChannelDecrement", Key::ChannelDecrement},
            {"MediaPlay", Key::MediaPlay},
            {"MediaPause", Key::MediaPause},
            {"MediaRecord", Key::MediaRecord},
            {"MediaFastForward", Key::MediaFastForward},
            {"MediaRewind", Key::MediaRewind},
            {"MediaNextTrack", Key::MediaNextTrack},
            {"MediaPreviousTrack", Key::MediaPreviousTrack},
            {"MediaStop", Key::MediaStop},
            {"MediaEject", Key::MediaEject},
            {"MediaPlayPause", Key::MediaPlayPause},
            {"MediaSelect", Key::MediaSelect},
            {"AcNew", Key::AcNew},
            {"AcOpen", Key::AcOpen},
            {"AcClose", Key::AcClose},
            {"AcExit", Key::AcExit},
            {"AcSave", Key::AcSave},
            {"AcPrint", Key::AcPrint},
            {"AcProperties", Key::AcProperties},
            {"AcSearch", Key::AcSearch},
            {"AcHome", Key::AcHome},
            {"AcBack", Key::AcBack},
            {"AcForward", Key::AcForward},
            {"AcStop", Key::AcStop},
            {"AcRefresh", Key::AcRefresh},
            {"AcBookmarks", Key::AcBookmarks},
            {"SoftLeft", Key::SoftLeft},
            {"SoftRight", Key::SoftRight},
            {"Call", Key::Call},
            {"EndCall", Key::EndCall}
        };

        sol::table keyCodeTable = lua.create_table();
        for (const auto& keyCode : keyCodes) {
            keyCodeTable[keyCode.first] = keyCode.second;
        }
        inputTable["keycode"] = keyCodeTable;
    }

    void BindMouseCodesToLua(sol::state& lua, sol::table& inputTable)
    {
        std::vector<std::pair<std::string, MouseCode>> mouseCodes = {
            {"Left", Mouse::ButtonLeft},
            {"Middle", Mouse::ButtonMiddle},
            {"Right", Mouse::ButtonRight},
            {"X1", Mouse::ButtonX1},
            {"X2", Mouse::ButtonX2}
        };

        sol::table mouseCodeTable = lua.create_table();
        for (const auto& mouseCode : mouseCodes) {
            mouseCodeTable[mouseCode.first] = mouseCode.second;
        }
        inputTable["mousecode"] = mouseCodeTable;
    }

    void BindControllerCodesToLua(sol::state& lua, sol::table& inputTable)
    {
        std::vector<std::pair<std::string, ControllerCode>> controllerCodes = {
            {"Invalid", Button::Invalid},
            {"South", Button::South},
            {"East", Button::East},
            {"West", Button::West},
            {"North", Button::North},
            {"Back", Button::Back},
            {"Guide", Button::Guide},
            {"Start", Button::Start},
            {"LeftStick", Button::LeftStick},
            {"RightStick", Button::RightStick},
            {"LeftShoulder", Button::LeftShoulder},
            {"RightShoulder", Button::RightShoulder},
            {"DpadUp", Button::DpadUp},
            {"DpadDown", Button::DpadDown},
            {"DpadLeft", Button::DpadLeft},
            {"DpadRight", Button::DpadRight},
            {"Misc1", Button::Misc1},
            {"RightPaddle1", Button::RightPaddle1},
            {"LeftPaddle1", Button::LeftPaddle1},
            {"RightPaddle2", Button::RightPaddle2},
            {"Leftpaddle2", Button::Leftpaddle2},
            {"Touchpad", Button::Touchpad},
            {"Misc2", Button::Misc2},
            {"Misc3", Button::Misc3},
            {"Misc4", Button::Misc4},
            {"Misc5", Button::Misc5},
            {"Misc6", Button::Misc6}
        };
        sol::table controllerCodeTable = lua.create_table();
        for (const auto& controllerCode : controllerCodes) {
            controllerCodeTable[controllerCode.first] = controllerCode.second;
        }
        inputTable["controllercode"] = controllerCodeTable;
    }

    void BindAxisCodesToLua(sol::state& lua, sol::table& inputTable)
    {
        std::vector<std::pair<std::string, AxisCode>> axisCodes = {
            {"Invalid", Axis::Invalid},
            {"LeftX", Axis::LeftX},
            {"LeftY", Axis::LeftY},
            {"RightX", Axis::RightX},
            {"RightY", Axis::RightY},
            {"LeftTrigger", Axis::LeftTrigger},
            {"RightTrigger", Axis::RightTrigger}
        };
        sol::table axisCodeTable = lua.create_table();
        for (const auto& axisCode : axisCodes) {
            axisCodeTable[axisCode.first] = axisCode.second;
        }
        inputTable["axiscode"] = axisCodeTable;
    }


    void BindInputActionsToLua(sol::state& lua, sol::table& inputTable)
    {
        // Actions
        std::vector<std::pair<std::string, InputAction>> actionCodes = {
            {"UiMoveHorizontal", ActionsEnum::UiMoveHorizontal},
            {"UiMoveVertical", ActionsEnum::UiMoveVertical},
            {"Confirm", ActionsEnum::Confirm},
            {"Cancel",  ActionsEnum::Cancel},
            {"MoveHorizontal", ActionsEnum::MoveHorizontal},
            {"MoveVertical", ActionsEnum::MoveVertical},
            {"AimHorizontal", ActionsEnum::AimHorizontal},
            {"AimVertical", ActionsEnum::AimVertical},
            {"Shoot", ActionsEnum::Shoot},
            {"Melee", ActionsEnum::Melee},
            {"Interact",  ActionsEnum::Interact},
            {"Dash", ActionsEnum::Dash},
            {"Cover", ActionsEnum::Cover},
            {"Skill1", ActionsEnum::Skill1},
            {"Skill2", ActionsEnum::Skill2},
            {"Skill3", ActionsEnum::Skill3},
            {"Injector",ActionsEnum::Injector},
            {"Grenade", ActionsEnum::Grenade},
            {"Map", ActionsEnum::Map},
            {"Pause", ActionsEnum::Pause}
        };

        sol::table actionCodeTable = lua.create_table();
        for (const auto& actionCode : actionCodes) {
            actionCodeTable[actionCode.first] = actionCode.second;
        }
        inputTable["action"] = actionCodeTable;

        // Button states
        std::vector<std::pair<std::string, ButtonState>> buttonStates = {
            {"Idle", ButtonStates::IDLE},
            {"Up", ButtonStates::UP},
            {"Down", ButtonStates::DOWN},
            {"Repeat", ButtonStates::REPEAT}
        };

        sol::table buttonStatesTable = lua.create_table();
        for (const auto& buttonState : buttonStates)
        {
            buttonStatesTable[buttonState.first] = buttonState.second;
        }

        inputTable["state"] = buttonStatesTable;

    }

    void LuaBackend::Initialize() {
        luaState.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);

        # pragma region Bind Log Functions
        luaState.set_function("log", [](const std::string& message) {
            COFFEE_CORE_INFO("{0}", message);
        });

        luaState.set_function("log_warning", [](const std::string& message) {
            COFFEE_CORE_WARN("{0}", message);
        });

        luaState.set_function("log_error", [](const std::string& message) {
            COFFEE_CORE_ERROR("{0}", message);
        });

        luaState.set_function("log_critical", [](const std::string& message) {
            COFFEE_CORE_CRITICAL("{0}", message);
        });
        # pragma endregion

        # pragma region Bind Input Functions
        sol::table inputTable = luaState.create_table();
        BindKeyCodesToLua(luaState, inputTable);
        BindMouseCodesToLua(luaState, inputTable);
        BindControllerCodesToLua(luaState, inputTable);
        BindAxisCodesToLua(luaState, inputTable);
        BindInputActionsToLua(luaState, inputTable);

        inputTable.set_function("is_key_pressed", [](KeyCode key) {
            return Input::IsKeyPressed(key);
        });

        inputTable.set_function("is_mouse_button_pressed", [](MouseCode button) {
            return Input::IsMouseButtonPressed(button);
        });

        inputTable.set_function("is_button_pressed", [](ButtonCode button) {
            return Input::GetButtonRaw(button);
        });

        inputTable.set_function("get_axis_position", [](AxisCode axis) {
            return Input::GetAxisRaw(axis);
        });

        inputTable.set_function("get_mouse_position", []() {
            glm::vec2 mousePosition = Input::GetMousePosition();
            return std::make_tuple(mousePosition.x, mousePosition.y);
        });

        inputTable.set_function("get_axis", [](InputAction action) {
            return Input::GetBinding(action).AsAxis(false);
        });

        inputTable.set_function("get_direction",[](InputAction action) {
            return Input::GetBinding(action).AsAxis(true);
        });

        inputTable.set_function("get_button", [](InputAction action) {
            return Input::GetBinding(action).AsButton();
        });

        luaState["Input"] = inputTable;
        # pragma endregion

        # pragma region Bind Timer and Stopwatch Functions

        // Bind Stopwatch class
        luaState.new_usertype<Stopwatch>("Stopwatch",
            sol::constructors<Stopwatch()>(),
            "start", &Stopwatch::Start,
            "stop", &Stopwatch::Stop,
            "reset", &Stopwatch::Reset,
            "get_elapsed_time", &Stopwatch::GetElapsedTime,
            "get_precise_elapsed_time", &Stopwatch::GetPreciseElapsedTime
        );

        // Bind Timer class
        luaState.new_usertype<Timer>("Timer",
            sol::constructors<Timer(), Timer(double, bool, bool, Timer::TimerCallback)>(),
            "start", &Timer::Start,
            "stop", &Timer::Stop,
            "set_wait_time", &Timer::setWaitTime,
            "get_wait_time", &Timer::getWaitTime,
            "set_one_shot", &Timer::setOneShot,
            "is_one_shot", &Timer::isOneShot,
            "set_auto_start", &Timer::setAutoStart,
            "is_auto_start", &Timer::isAutoStart,
            "set_paused", &Timer::setPaused,
            "is_paused", &Timer::isPaused,
            "get_time_left", &Timer::GetTimeLeft,
            "set_callback", [](Timer& timer, const sol::protected_function& callback) {
                timer.SetCallback([callback] {
                    if (const auto result = callback(); !result.valid()) {
                        const sol::error err = result;
                        COFFEE_CORE_ERROR("Timer callback error: {0}", err.what());
                    }
                });
            }
        );

        // Helper function to create a timer
        luaState.set_function("create_timer", [](double waitTime, bool autoStart, bool oneShot, sol::protected_function callback) {
            return Timer(waitTime, autoStart, oneShot, [callback]() {
                auto result = callback();
                if (!result.valid()) {
                    sol::error err = result;
                    COFFEE_CORE_ERROR("Timer callback error: {0}", err.what());
                }
            });
        });

        # pragma endregion

        # pragma region Bind GLM Functions
        luaState.new_usertype<glm::vec2>("Vector2",
            sol::constructors<glm::vec2(), glm::vec2(float), glm::vec2(float, float)>(),
            "x", &glm::vec2::x,
            "y", &glm::vec2::y,
            "normalize", [](const glm::vec2& a) { return glm::normalize(a); },
            "length", [](const glm::vec2& a) { return glm::length(a); },
            "length_squared", [](const glm::vec2& a) { return glm::length2(a); },
            "distance_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance(a, b); },
            "distance_squared_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::distance2(a, b); },
            "lerp", [](const glm::vec2& a, const glm::vec2& b, float t) { return glm::mix(a, b, t); },
            "dot", [](const glm::vec2& a, const glm::vec2& b) { return glm::dot(a, b); },
            "angle_to", [](const glm::vec2& a, const glm::vec2& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
            "max", [](const glm::vec2& a, const glm::vec2& b) { return (glm::max)(a, b); },
            "min", [](const glm::vec2& a, const glm::vec2& b) { return (glm::min)(a, b); },
            "abs", [](const glm::vec2& a) { return glm::abs(a); }
            //TODO: Add more functions
        );

        luaState.new_usertype<glm::vec3>("Vector3",
            sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z,
            "cross", [](const glm::vec3& a, const glm::vec3& b) { return glm::cross(a, b); },
            "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
            "normalize", [](const glm::vec3& a) { return glm::normalize(a); },
            "length", [](const glm::vec3& a) { return glm::length(a); },
            "length_squared", [](const glm::vec3& a) { return glm::length2(a); },
            "distance_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance(a, b); },
            "distance_squared_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::distance2(a, b); },
            "lerp", [](const glm::vec3& a, const glm::vec3& b, float t) { return glm::mix(a, b, t); },
            "dot", [](const glm::vec3& a, const glm::vec3& b) { return glm::dot(a, b); },
            "angle_to", [](const glm::vec3& a, const glm::vec3& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
            "max", [](const glm::vec3& a, const glm::vec3& b) { return (glm::max)(a, b); },
            "min", [](const glm::vec3& a, const glm::vec3& b) { return (glm::min)(a, b); },
            "abs", [](const glm::vec3& a) { return glm::abs(a); }
            //TODO: Add more functions
        );

        luaState.new_usertype<glm::vec4>("Vector4",
            sol::constructors<glm::vec4(), glm::vec4(float), glm::vec4(float, float, float, float)>(),
            "x", &glm::vec4::x,
            "y", &glm::vec4::y,
            "z", &glm::vec4::z,
            "w", &glm::vec4::w,
            "normalize", [](const glm::vec4& a) { return glm::normalize(a); },
            "length", [](const glm::vec4& a) { return glm::length(a); },
            "length_squared", [](const glm::vec4& a) { return glm::length2(a); },
            "distance_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::distance(a, b); },
            "distance_squared_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::distance2(a, b); },
            "lerp", [](const glm::vec4& a, const glm::vec4& b, float t) { return glm::mix(a, b, t); },
            "dot", [](const glm::vec4& a, const glm::vec4& b) { return glm::dot(a, b); },
            "angle_to", [](const glm::vec4& a, const glm::vec4& b) { return glm::degrees(glm::acos(glm::dot(glm::normalize(a), glm::normalize(b)))); },
            "max", [](const glm::vec4& a, const glm::vec4& b) { return (glm::max)(a, b); },
            "min", [](const glm::vec4& a, const glm::vec4& b) { return (glm::min)(a, b); },
            "abs", [](const glm::vec4& a) { return glm::abs(a); }
            //TODO: Add more functions
        );

        luaState.new_usertype<glm::mat4>("Mat4",
            sol::constructors<glm::mat4(), glm::mat4(float)>(),
            "identity", []() { return glm::mat4(1.0f); },
            "inverse", [](const glm::mat4& mat) { return glm::inverse(mat); },
            "transpose", [](const glm::mat4& mat) { return glm::transpose(mat); },
            "translate", [](const glm::mat4& mat, const glm::vec3& vec) { return glm::translate(mat, vec); },
            "rotate", [](const glm::mat4& mat, float angle, const glm::vec3& axis) { return glm::rotate(mat, angle, axis); },
            "scale", [](const glm::mat4& mat, const glm::vec3& vec) { return glm::scale(mat, vec); },
            "perspective", [](float fovy, float aspect, float nearPlane, float farPlane) { return glm::perspective(fovy, aspect, nearPlane, farPlane); },
            "ortho", [](float left, float right, float bottom, float top, float zNear, float zFar) { return glm::ortho(left, right, bottom, top, zNear, zFar); }
        );

        luaState.new_usertype<glm::quat>("Quaternion",
            sol::constructors<glm::quat(), glm::quat(float, float, float, float), glm::quat(const glm::vec3&), glm::quat(float, const glm::vec3&)>(),
            "x", &glm::quat::x,
            "y", &glm::quat::y,
            "z", &glm::quat::z,
            "w", &glm::quat::w,
            "from_euler", [](const glm::vec3& euler) { return glm::quat(glm::radians(euler)); },
            "to_euler_angles", [](const glm::quat& q) { return glm::eulerAngles(q); },
            "toMat4", [](const glm::quat& q) { return glm::toMat4(q); },
            "normalize", [](const glm::quat& q) { return glm::normalize(q); },
            "slerp", [](const glm::quat& a, const glm::quat& b, float t) { return glm::slerp(a, b, t); }
        );
        # pragma endregion

        #pragma region Bind Entity Functions
        luaState.new_usertype<Entity>("Entity",
            sol::constructors<Entity(), Entity(entt::entity, Scene*)>(),
            "add_component", [](Entity* self, const std::string& componentName) {
                if (componentName == "TagComponent") {
                    self->AddComponent<TagComponent>();
                } else if (componentName == "TransformComponent") {
                    self->AddComponent<TransformComponent>();
                } else if (componentName == "CameraComponent") {
                    self->AddComponent<CameraComponent>();
                } else if (componentName == "MeshComponent") {
                    self->AddComponent<MeshComponent>();
                } else if (componentName == "MaterialComponent") {
                    self->AddComponent<MaterialComponent>();
                } else if (componentName == "LightComponent") {
                    self->AddComponent<LightComponent>();
                } else if (componentName == "ScriptComponent") {
                    self->AddComponent<ScriptComponent>();
                } else if (componentName == "UIImageComponent") {
                    self->AddComponent<UIImageComponent>();
                } else if (componentName == "UITextComponent") {
                    self->AddComponent<UITextComponent>();
                } else if (componentName == "UISliderComponent"){
                    self->AddComponent<UISliderComponent>();
                }else if (componentName == "UIButtonComponent"){
                    self->AddComponent<UIButtonComponent>();
                }else if (componentName == "UIToggleComponent")
                {
                    self->AddComponent<UIToggleComponent>();
                }else if (componentName == "UIAnchorPosition"){
                    self->AddComponent<UIAnchorPosition>();
                }else if (componentName == "ParticlesSystemComponent"){
                    self->AddComponent<ParticlesSystemComponent>();
                } else if (componentName == "AudioSourceComponent") {
                    self->AddComponent<AudioSourceComponent>();
                }
            },
            "get_component", [this](Entity* self, const std::string& componentName) -> sol::object {
                if (componentName == "TagComponent") {
                    return sol::make_object(luaState, self->GetComponent<TagComponent>());
                } else if (componentName == "TransformComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<TransformComponent>()));
                } else if (componentName == "CameraComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<CameraComponent>()));
                } else if (componentName == "MeshComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<MeshComponent>()));
                } else if (componentName == "MaterialComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<MaterialComponent>()));
                } else if (componentName == "LightComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<LightComponent>()));
                } else if (componentName == "ScriptComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<ScriptComponent>()));
                } else if (componentName == "UIImageComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<UIImageComponent>()));
                } else if (componentName == "UITextComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<UITextComponent>()));
                }else if (componentName == "UISliderComponent"){
                    return sol::make_object(luaState, std::ref(self->GetComponent<UISliderComponent>()));
                }else if (componentName == "UIButtonComponent"){
                    return sol::make_object(luaState, std::ref(self->GetComponent<UIButtonComponent>()));
                }else if (componentName == "UIAnchorPosition"){
                    return sol::make_object(luaState, std::ref(self->GetComponent<UIAnchorPosition>()));
                }else if (componentName == "UIToggleComponent"){
                    return sol::make_object(luaState, std::ref(self->GetComponent<UIToggleComponent>()));
                }else if (componentName == "ParticlesSystemComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<ParticlesSystemComponent>()));
                } else if (componentName == "NavigationAgentComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<NavigationAgentComponent>()));
                } else if (componentName == "RigidbodyComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<RigidbodyComponent>()));
                } else if (componentName == "AudioSourceComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<AudioSourceComponent>()));
                } else if (componentName == "AnimatorComponent") {
                    return sol::make_object(luaState, std::ref(self->GetComponent<AnimatorComponent>()));
                }

                return sol::nil;
            },
            "has_component", [](Entity* self, const std::string& componentName) -> bool {
                if (componentName == "TagComponent") {
                    return self->HasComponent<TagComponent>();
                } else if (componentName == "TransformComponent") {
                    return self->HasComponent<TransformComponent>();
                } else if (componentName == "CameraComponent") {
                    return self->HasComponent<CameraComponent>();
                } else if (componentName == "MeshComponent") {
                    return self->HasComponent<MeshComponent>();
                } else if (componentName == "MaterialComponent") {
                    return self->HasComponent<MaterialComponent>();
                } else if (componentName == "LightComponent") {
                    return self->HasComponent<LightComponent>();
                } else if (componentName == "ScriptComponent") {
                    return self->HasComponent<ScriptComponent>();
                } else if (componentName == "UIImageComponent") {
                    return self->HasComponent<UIImageComponent>();
                } else if (componentName == "UITextComponent") {
                    return self->HasComponent<UITextComponent>();
                }
                else if (componentName == "UISliderComponent"){
                    return self->HasComponent<UISliderComponent>();
                }else if (componentName == "UIButtonComponent"){
                    return self->HasComponent<UIButtonComponent>();
                }else if (componentName == "UIAnchorPosition"){
                    return self->HasComponent<UIAnchorPosition>();
                }else if (componentName == "UIToggleComponent"){
                    return self->HasComponent<UIToggleComponent>();
                }else if (componentName == "ParticlesSystemComponent") {
                    return self->HasComponent<ParticlesSystemComponent>();
                } else if (componentName == "NavigationAgentComponent") {
                    return self->HasComponent<NavigationAgentComponent>();
                } else if (componentName == "RigidbodyComponent") {
                    return self->HasComponent<RigidbodyComponent>();
                } else if (componentName == "AnimatorComponent") {
                    return self->HasComponent<AnimatorComponent>();
                } else if (componentName == "AudioSourceComponent") {
                    return self->HasComponent<AudioSourceComponent>();
                }
                return false;
            },
            "remove_component", [](Entity* self, const std::string& componentName) {
                if (componentName == "TagComponent") {
                    self->RemoveComponent<TagComponent>();
                } else if (componentName == "TransformComponent") {
                    self->RemoveComponent<TransformComponent>();
                } else if (componentName == "CameraComponent") {
                    self->RemoveComponent<CameraComponent>();
                } else if (componentName == "MeshComponent") {
                    self->RemoveComponent<MeshComponent>();
                } else if (componentName == "MaterialComponent") {
                    self->RemoveComponent<MaterialComponent>();
                } else if (componentName == "LightComponent") {
                    self->RemoveComponent<LightComponent>();
                } else if (componentName == "ScriptComponent") {
                    self->RemoveComponent<ScriptComponent>();

                } else if (componentName == "UIImageComponent") {
                    self->RemoveComponent<UIImageComponent>();
                } else if (componentName == "UITextComponent") {
                    self->RemoveComponent<UITextComponent>();
                }else if (componentName == "UIAnchorPosition") {
                    self->RemoveComponent<UIAnchorPosition>();
                }else if (componentName == "UIButtonComponent") {
                    self->RemoveComponent<UIButtonComponent>();
                } else if (componentName == "UITextComponent") {
                    self->RemoveComponent<UITextComponent>();
                }
                else if (componentName == "UISliderComponent") {
                            self->RemoveComponent<UISliderComponent>();
                }
                else if (componentName == "UIToggleComponent") {
                            self->RemoveComponent<UIToggleComponent>();
                }
                else if (componentName == "ParticlesSystemComponent") {
                    
                    self->RemoveComponent<ParticlesSystemComponent>();

                } else if (componentName == "RigidbodyComponent") {
                    self->RemoveComponent<RigidbodyComponent>();
                } else if (componentName == "AudioSourceComponent") {
                    self->RemoveComponent<AudioSourceComponent>();
                }
            },
            "set_parent", &Entity::SetParent,
            "get_parent", &Entity::GetParent,
            "get_next_sibling", &Entity::GetNextSibling,
            "get_prev_sibling", &Entity::GetPrevSibling,
            "get_child", &Entity::GetChild,
            "get_children", &Entity::GetChildren,
            "is_valid", [](Entity* self) { return static_cast<bool>(*self); }
        );
        #pragma endregion

        # pragma region Bind Components Functions
        luaState.new_usertype<TagComponent>("TagComponent",
            sol::constructors<TagComponent(), TagComponent(const std::string&)>(),
            "tag", &TagComponent::Tag
        );

        luaState.new_usertype<TransformComponent>("TransformComponent",
            sol::constructors<TransformComponent(), TransformComponent(const glm::vec3&)>(),
            "position", &TransformComponent::Position,
            "rotation", &TransformComponent::Rotation,
            "scale", &TransformComponent::Scale,
            "get_local_transform", &TransformComponent::GetLocalTransform,
            "set_local_transform", &TransformComponent::SetLocalTransform,
            "get_world_transform", &TransformComponent::GetWorldTransform,
            "set_world_transform", &TransformComponent::SetWorldTransform
        );

        luaState.new_usertype<CameraComponent>("CameraComponent",
            sol::constructors<CameraComponent()>(),
            "camera", &CameraComponent::Camera
        );

        luaState.new_usertype<MeshComponent>("MeshComponent",
            sol::constructors<MeshComponent(), MeshComponent(Ref<Mesh>)>(),
            "mesh", &MeshComponent::mesh,
            "drawAABB", &MeshComponent::drawAABB,
            "get_mesh", &MeshComponent::GetMesh
        );

        luaState.new_usertype<MaterialComponent>("MaterialComponent",
            sol::constructors<MaterialComponent(), MaterialComponent(Ref<Material>)>(),
            "material", &MaterialComponent::material
        );

        luaState.new_usertype<LightComponent>("LightComponent",
            sol::constructors<LightComponent()>(),
            "color", &LightComponent::Color,
            "direction", &LightComponent::Direction,
            "position", &LightComponent::Position,
            "range", &LightComponent::Range,
            "attenuation", &LightComponent::Attenuation,
            "intensity", &LightComponent::Intensity,
            "angle", &LightComponent::Angle,
            "type", &LightComponent::type
        );

        luaState.new_usertype<NavigationAgentComponent>("NavigationAgentComponent",
            sol::constructors<NavigationAgentComponent()>(),
            "path", &NavigationAgentComponent::Path,
            "find_path", &NavigationAgentComponent::FindPath
        );

        luaState.new_usertype<ScriptComponent>("ScriptComponent",
            sol::constructors<ScriptComponent(), ScriptComponent(const std::filesystem::path& path, ScriptingLanguage language)>(),
            sol::meta_function::index, [](ScriptComponent& self, const std::string& key) {
                return std::dynamic_pointer_cast<LuaScript>(self.script)->GetVariable<sol::object>(key);
            },
            sol::meta_function::new_index, [](ScriptComponent& self, const std::string& key, sol::object value) {
                std::dynamic_pointer_cast<LuaScript>(self.script)->SetVariable(key, value);
            },
            sol::meta_function::call, [](ScriptComponent& self, const std::string& functionName, sol::variadic_args args) {
                std::dynamic_pointer_cast<LuaScript>(self.script)->CallFunction(functionName);
            }
        );

        luaState.new_usertype<UIImageComponent>("UIImageComponent",
            sol::constructors<UIImageComponent(), UIImageComponent(const std::string&, const glm::vec2&, bool) >(),
            "get_size", &UIImageComponent::Size,
            "set_size", [](UIImageComponent& self, const glm::vec2& size) { self.Size = size; },
            "is_visible", &UIImageComponent::Visible,
            "set_visible", [](UIImageComponent& self, bool visible) { self.Visible = visible; },
            "get_layer", &UIImageComponent::Layer,
            "set_layer", [](UIImageComponent& self, int layer) { self.Layer = layer; },
            "set_texture", sol::overload(
                [](UIImageComponent& self, const Ref<Texture2D>& newTexture) { self.SetTexture(newTexture); },
                [](UIImageComponent& self, const std::string& texturePath) { self.SetTexture(texturePath); }
            )
        );

        luaState.new_usertype<UITextComponent>("UITextComponent",
                sol::constructors<UITextComponent(), UITextComponent(const std::string&, const std::string&, float, const glm::vec4&)>(),
                "get_text", &UITextComponent::Text,
                "set_text", [](UITextComponent& self, const std::string& text) { self.Text = text; },
                "get_font_path", &UITextComponent::FontPath,
                "set_font_path", [](UITextComponent& self, const std::string& fontPath) { self.FontPath = fontPath; },
                "get_font_size", &UITextComponent::FontSize,
                "set_font_size", [](UITextComponent& self, float fontSize) { self.FontSize = fontSize; },
                "get_color", &UITextComponent::Color,
                "set_color", [](UITextComponent& self, const glm::vec4& color) { self.Color = color; },
               "get_layer", &UIImageComponent::Layer,
               "set_layer", [](UIImageComponent& self, int layer) { self.Layer = layer; },
                "is_visible", &UITextComponent::Visible,
                "set_visible", [](UITextComponent& self, bool visible) { self.Visible = visible; }
            );


        luaState.new_enum<UIAnchorPosition>("UIAnchorPosition",
                                            {
                                                {"TopLeft", UIAnchorPosition::TopLeft},
                                                {"TopCenter", UIAnchorPosition::TopCenter},
                                                {"TopRight", UIAnchorPosition::TopRight},
                                                {"CenterLeft", UIAnchorPosition::CenterLeft},
                                                {"Center", UIAnchorPosition::Center},
                                                {"CenterRight", UIAnchorPosition::CenterRight},
                                                {"BottomLeft", UIAnchorPosition::BottomLeft},
                                                {"BottomCenter", UIAnchorPosition::BottomCenter},
                                                {"BottomRight", UIAnchorPosition::BottomRight}
                                            }
        );



            luaState.new_usertype<UIButtonComponent>("UIButtonComponent",
        sol::constructors<UIButtonComponent(), UIButtonComponent(const std::string&, const std::string&, const std::string&)>(),

        "set_state", [](UIButtonComponent& self, const std::string& state) {
            if (state == "Base") {
                self.SetState(UIButtonComponent::ButtonState::Base);
            } else if (state == "Selected") {
                self.SetState(UIButtonComponent::ButtonState::Selected);
            } else if (state == "Pressed") {
                self.SetState(UIButtonComponent::ButtonState::Pressed);
            } else {
                COFFEE_CORE_WARN("Invalid button state: {0}", state);
            }
        },
        "get_state", [](UIButtonComponent& self) -> std::string {
            switch (self.CurrentState) {
                case UIButtonComponent::ButtonState::Base: return "Base";
                case UIButtonComponent::ButtonState::Selected: return "Selected";
                case UIButtonComponent::ButtonState::Pressed: return "Pressed";
                default: return "Unknown";
            }
        },


        "set_visible", &UIButtonComponent::Visible,
        "is_visible", &UIButtonComponent::Visible,

        "set_base_texture", [](UIButtonComponent& self, const std::string& texturePath) {
            if (!texturePath.empty()) {
                self.BaseTexture = Texture2D::Load(texturePath);
            }
        },
        "set_selected_texture", [](UIButtonComponent& self, const std::string& texturePath) {
            if (!texturePath.empty()) {
                self.SelectedTexture = Texture2D::Load(texturePath);
            }
        },
        "set_pressed_texture", [](UIButtonComponent& self, const std::string& texturePath) {
            if (!texturePath.empty()) {
                self.PressedTexture = Texture2D::Load(texturePath);
            }
        },

        "set_base_size", [](UIButtonComponent& self, float width, float height) {
            self.BaseSize = {width, height};
        },
        "set_selected_size", [](UIButtonComponent& self, float width, float height) {
            self.SelectedSize = {width, height};
        },
        "set_pressed_size", [](UIButtonComponent& self, float width, float height) {
            self.PressedSize = {width, height};
        },

        "get_current_size", [](UIButtonComponent& self) -> sol::table {
            auto size = self.GetCurrentSize();
            sol::table sizeTable = luaState.create_table();
            sizeTable[1] = size.x;
            sizeTable[2] = size.y;
            return sizeTable;
        },

        "set_base_color", [](UIButtonComponent& self, float r, float g, float b, float a) {
            self.BaseColor = {r, g, b, a};
        },
        "set_selected_color", [](UIButtonComponent& self, float r, float g, float b, float a) {
            self.SelectedColor = {r, g, b, a};
        },
        "set_pressed_color", [](UIButtonComponent& self, float r, float g, float b, float a) {
            self.PressedColor = {r, g, b, a};
        },

        "get_current_color", [](UIButtonComponent& self) -> sol::table {
            auto color = self.GetCurrentColor();
            sol::table colorTable = luaState.create_table();
            colorTable[1] = color.r;
            colorTable[2] = color.g;
            colorTable[3] = color.b;
            colorTable[4] = color.a;
            return colorTable;
        },
         "get_layer", &UIImageComponent::Layer,
         "set_layer", [](UIImageComponent& self, int layer) { self.Layer = layer; },

        "update", &UIButtonComponent::Update
    );

        luaState.new_usertype<UISliderComponent>("UISliderComponent",
        sol::constructors<UISliderComponent()>(),
        "get_bar_texture", [](UISliderComponent& self) { return self.BarTexture; },
        "set_bar_texture", [](UISliderComponent& self, const Ref<Texture2D>& texture) { self.BarTexture = texture; },
        "get_handle_texture", [](UISliderComponent& self) { return self.HandleTexture; },
        "set_handle_texture", [](UISliderComponent& self, const Ref<Texture2D>& texture) { self.HandleTexture = texture; },
        "get_size", [](UISliderComponent& self) { return self.Size; },
        "set_size", [](UISliderComponent& self, const glm::vec2& size) { self.Size = size; },
        "get_handle_size", [](UISliderComponent& self) { return self.HandleSize; },
        "set_handle_size", [](UISliderComponent& self, const glm::vec2& size) { self.HandleSize = size; },
        "get_value", [](UISliderComponent& self) { return self.Value; },
        "set_value", [](UISliderComponent& self, float value) { self.Value = glm::clamp(value, 0.0f, 1.0f); },
        "is_visible", [](UISliderComponent& self) { return self.Visible; },
        "set_visible", [](UISliderComponent& self, bool visible) { self.Visible = visible; },
         "get_layer", &UIImageComponent::Layer,
         "set_layer", [](UIImageComponent& self, int layer) { self.Layer = layer; },
        "on_value_changed", [](UISliderComponent& self, sol::function callback){}
        );

        luaState.new_usertype<UIToggleComponent>("UIToggleComponent",
            sol::constructors<UIToggleComponent(), UIToggleComponent(const std::string&, const std::string&, const glm::vec2&, bool)>(),
            "is_active", &UIToggleComponent::IsActive,
            "set_active", [](UIToggleComponent& self, bool active) { self.IsActive = active; },
            "toggle", &UIToggleComponent::Toggle,
            "is_visible", &UIToggleComponent::Visible,
            "set_visible", [](UIToggleComponent& self, bool visible) { self.Visible = visible; },
            "get_size", &UIToggleComponent::Size,
            "set_size", [](UIToggleComponent& self, const glm::vec2& size) { self.Size = size; },
             "get_layer", &UIImageComponent::Layer,
             "set_layer", [](UIImageComponent& self, int layer) { self.Layer = layer; },
            "get_active_texture", &UIToggleComponent::ActiveTexture,
            "set_active_texture", [](UIToggleComponent& self, const Ref<Texture2D>& texture) { self.ActiveTexture = texture; },
            "get_inactive_texture", &UIToggleComponent::InactiveTexture,
            "set_inactive_texture", [](UIToggleComponent& self, const Ref<Texture2D>& texture) { self.InactiveTexture = texture; }
        );

        luaState.new_usertype<ParticlesSystemComponent>("ParticlesSystemComponent", sol::constructors<ParticlesSystemComponent()>(), 
            "emit",&ParticlesSystemComponent::Emit, 
            "set_looping",&ParticlesSystemComponent::SetLooping, 
            "get_emitter", &ParticlesSystemComponent::GetParticleEmitter
            );

        luaState.new_usertype<RigidbodyComponent>("RigidbodyComponent",
            "rb", &RigidbodyComponent::rb,
            "on_collision_enter", [](RigidbodyComponent& self, sol::protected_function fn) {
                self.callback.OnCollisionEnter([fn](CollisionInfo& info) {
                    fn(info.entityA, info.entityB);
                });
            },
            "on_collision_stay", [](RigidbodyComponent& self, sol::protected_function fn) {
                self.callback.OnCollisionStay([fn](CollisionInfo& info) {
                    fn(info.entityA, info.entityB);
                });
            },
            "on_collision_exit", [](RigidbodyComponent& self, sol::protected_function fn) {
                self.callback.OnCollisionExit([fn](CollisionInfo& info) {
                    fn(info.entityA, info.entityB);
                });
            }
        );

        luaState.new_usertype<AnimatorComponent>(
            "AnimatorComponent", sol::constructors<AnimatorComponent(), AnimatorComponent()>(),
            "set_current_animation", &AnimatorComponent::SetCurrentAnimation
        );

        luaState.new_usertype<AudioSourceComponent>("AudioSourceComponent",
        sol::constructors<AudioSourceComponent(), AudioSourceComponent()>(),
         "set_volume", &AudioSourceComponent::SetVolume,
         "play", &AudioSourceComponent::Play,
         "pause", &AudioSourceComponent::Stop);


        # pragma endregion

        # pragma region Bind Scene Functions

        luaState.new_usertype<Scene>("Scene",
            "create_entity", &Scene::CreateEntity,
            "destroy_entity", &Scene::DestroyEntity,
            "duplicate_entity", &Scene::Duplicate,
            "get_entity_by_name", &Scene::GetEntityByName,
            "get_all_entities", &Scene::GetAllEntities
        );

        luaState.new_usertype<SceneManager>("SceneManager",
            "preload_scene", [](const std::string& scenePath) {
                return SceneManager::PreloadScene(scenePath);
            },
            "preload_scene_async", [](const std::string& scenePath) {
                return SceneManager::PreloadSceneAsync(scenePath);
            },
            "change_scene", sol::overload(
                [](const std::string& scenePath) {
                    AudioZone::RemoveAllReverbZones();
                    Audio::UnregisterAllGameObjects();
                    SceneManager::ChangeScene(scenePath);
                },
                [](const Ref<Scene>& scene) {
                    AudioZone::RemoveAllReverbZones();
                    Audio::UnregisterAllGameObjects();
                    SceneManager::ChangeScene(scene);
                }
            ),
            "change_scene_async", [](const std::string& scenePath) {
                SceneManager::ChangeSceneAsync(scenePath);
            }
        );
        

        # pragma endregion

        # pragma region Bind Physics Functions

        // Bind RigidBody::Type enum
        luaState.new_enum<RigidBody::Type>("RigidBodyType",
        {
            {"Static", RigidBody::Type::Static},
            {"Dynamic", RigidBody::Type::Dynamic},
            {"Kinematic", RigidBody::Type::Kinematic}
        });

        // Bind RigidBody properties
        luaState.new_usertype<RigidBody::Properties>("RigidBodyProperties",
            sol::constructors<RigidBody::Properties()>(),
            "type", &RigidBody::Properties::type,
            "mass", &RigidBody::Properties::mass,
            "useGravity", &RigidBody::Properties::useGravity,
            "freezeX", &RigidBody::Properties::freezeX,
            "freezeY", &RigidBody::Properties::freezeY,
            "freezeZ", &RigidBody::Properties::freezeZ,
            "freezeRotX", &RigidBody::Properties::freezeRotX,
            "freezeRotY", &RigidBody::Properties::freezeRotY,
            "freezeRotZ", &RigidBody::Properties::freezeRotZ,
            "isTrigger", &RigidBody::Properties::isTrigger,
            "velocity", &RigidBody::Properties::velocity,
            "friction", &RigidBody::Properties::friction,
            "linearDrag", &RigidBody::Properties::linearDrag,
            "angularDrag", &RigidBody::Properties::angularDrag
        );

        // Bind RigidBody methods
        luaState.new_usertype<RigidBody>("RigidBody",
            // Position and rotation
            "set_position", &RigidBody::SetPosition,
            "get_position", &RigidBody::GetPosition,
            "set_rotation", &RigidBody::SetRotation,
            "get_rotation", &RigidBody::GetRotation,

            // Velocity and forces
            "set_velocity", &RigidBody::SetVelocity,
            "get_velocity", &RigidBody::GetVelocity,
            "add_velocity", &RigidBody::AddVelocity,
            "apply_force", &RigidBody::ApplyForce,
            "apply_impulse", &RigidBody::ApplyImpulse,
            "reset_velocity", &RigidBody::ResetVelocity,
            "clear_forces", &RigidBody::ClearForces,

            // Torque and angular velocity methods
            "apply_torque", &RigidBody::ApplyTorque,
            "apply_torque_impulse", &RigidBody::ApplyTorqueImpulse,
            "set_angular_velocity", &RigidBody::SetAngularVelocity,
            "get_angular_velocity", &RigidBody::GetAngularVelocity,

            // Collisions and triggers
            "set_trigger", &RigidBody::SetTrigger,

            // Body type
            "get_body_type", &RigidBody::GetBodyType,
            "set_body_type", &RigidBody::SetBodyType,

            // Mass
            "get_mass", &RigidBody::GetMass,
            "set_mass", &RigidBody::SetMass,

            // Gravity
            "get_use_gravity", &RigidBody::GetUseGravity,
            "set_use_gravity", &RigidBody::SetUseGravity,

            // Constraints
            "get_freeze_x", &RigidBody::GetFreezeX,
            "set_freeze_x", &RigidBody::SetFreezeX,
            "get_freeze_y", &RigidBody::GetFreezeY,
            "set_freeze_y", &RigidBody::SetFreezeY,
            "get_freeze_z", &RigidBody::GetFreezeZ,
            "set_freeze_z", &RigidBody::SetFreezeZ,
            "get_freeze_rot_x", &RigidBody::GetFreezeRotX,
            "set_freeze_rot_x", &RigidBody::SetFreezeRotX,
            "get_freeze_rot_y", &RigidBody::GetFreezeRotY,
            "set_freeze_rot_y", &RigidBody::SetFreezeRotY,
            "get_freeze_rot_z", &RigidBody::GetFreezeRotZ,
            "set_freeze_rot_z", &RigidBody::SetFreezeRotZ,

            // Physical properties
            "get_friction", &RigidBody::GetFriction,
            "set_friction", &RigidBody::SetFriction,
            "get_linear_drag", &RigidBody::GetLinearDrag,
            "set_linear_drag", &RigidBody::SetLinearDrag,
            "get_angular_drag", &RigidBody::GetAngularDrag,
            "set_angular_drag", &RigidBody::SetAngularDrag,

            // Utility
            "get_is_trigger", &RigidBody::GetIsTrigger
        );

        // Add Collider usertype bindings
        luaState.new_usertype<Collider>("Collider");

        luaState.new_usertype<BoxCollider>("BoxCollider",
            sol::constructors<BoxCollider(), BoxCollider(const glm::vec3&)>(),
            sol::base_classes, sol::bases<Collider>()
        );

        luaState.new_usertype<SphereCollider>("SphereCollider",
            sol::constructors<SphereCollider(), SphereCollider(float)>(),
            sol::base_classes, sol::bases<Collider>()
        );

        luaState.new_usertype<CapsuleCollider>("CapsuleCollider",
            sol::constructors<CapsuleCollider(), CapsuleCollider(float, float)>(),
            sol::base_classes, sol::bases<Collider>()
        );

        // Helper functions for creating colliders and rigidbodies
        luaState.set_function("create_box_collider", [](const glm::vec3& size) {
            return CreateRef<BoxCollider>(size);
        });

        luaState.set_function("create_sphere_collider", [](float radius) {
            return CreateRef<SphereCollider>(radius);
        });

        luaState.set_function("create_capsule_collider", [](float radius, float height) {
            return CreateRef<CapsuleCollider>(radius, height);
        });

        luaState.set_function("create_rigidbody", [](const RigidBody::Properties& props, const Ref<Collider>& collider) {
            return RigidBody::Create(props, collider);
        });

        sol::table physicsTable = luaState.create_table();
        luaState["Physics"] = physicsTable;

        // Bind RaycastHit type to Lua
        luaState.new_usertype<RaycastHit>(
            "RaycastHit",
            "hasHit", &RaycastHit::hasHit,
            "hitEntity", &RaycastHit::hitEntity,
            "hitPoint", &RaycastHit::hitPoint,
            "hitNormal", &RaycastHit::hitNormal,
            "hitFraction", &RaycastHit::hitFraction
        );

        // Bind Raycast functions
        physicsTable["Raycast"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> RaycastHit {
            auto scene = SceneManager::GetActiveScene();
            if (!scene)
                return RaycastHit{};

            return scene->GetPhysicsWorld().Raycast(origin, direction, maxDistance);
        };

        physicsTable["RaycastAll"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> std::vector<RaycastHit> {
            auto scene = SceneManager::GetActiveScene();
            if (!scene)
                return std::vector<RaycastHit>{};

            return scene->GetPhysicsWorld().RaycastAll(origin, direction, maxDistance);
        };

        physicsTable["RaycastAny"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance) -> bool {
            auto scene = SceneManager::GetActiveScene();
            if (!scene)
                return false;

            return scene->GetPhysicsWorld().RaycastAny(origin, direction, maxDistance);
        };

        physicsTable["DebugDrawRaycast"] = [](const glm::vec3& origin, const glm::vec3& direction, float maxDistance,
            sol::optional<glm::vec4> rayColor, sol::optional<glm::vec4> hitColor) {
            auto scene = SceneManager::GetActiveScene();
            if (!scene)
            return;

            // Default colors if not provided
            glm::vec4 rColor = rayColor.value_or(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
            glm::vec4 hColor = hitColor.value_or(glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));

            scene->GetPhysicsWorld().DebugDrawRaycast(origin, direction, maxDistance, rColor, hColor);
        };

        # pragma endregion
    }

    Ref<Script> LuaBackend::CreateScript(const std::filesystem::path& path) {
        return CreateRef<LuaScript>(path);
    }

    void LuaBackend::ExecuteScript(Script& script) {
        LuaScript& luaScript = static_cast<LuaScript&>(const_cast<Script&>(script));
        try {
            luaState.script_file(luaScript.GetPath().string(), luaScript.GetEnvironment());
        } catch (const sol::error& e) {
            COFFEE_CORE_ERROR("Lua: {0}", e.what());
        }
    }

} // namespace Coffee