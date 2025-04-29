#include "LuaInput.h"

#include "CoffeeEngine/Core/Input.h"
#include "CoffeeEngine/Core/ControllerCodes.h"
#include "CoffeeEngine/Core/KeyCodes.h"
#include "CoffeeEngine/Core/MouseCodes.h"

namespace Coffee
{
    void BindKeyCodesToLua(sol::state& lua, sol::table& inputTable)
    {
        std::vector<std::pair<std::string, Coffee::KeyCode>> keyCodes = {
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
        // DEPRECATED - Temporary conversion table for default input map
        // Old actions - Old identifiers -> New identifiers
        std::vector<std::pair<std::string, std::string>> actionCodes = {
            {"UiMoveHorizontal", "UiX"},
            {"UiMoveVertical", "UiY"},
            {"Confirm", "Cancel"},
            {"Cancel",  "Confirm"},
            {"MoveHorizontal", "MoveX"},
            {"MoveVertical", "MoveY"},
            {"AimHorizontal", "AimX"},
            {"AimVertical", "AimY"},
            {"Shoot", "Shoot"},
            {"Melee", "Melee"},
            {"Interact",  "Interact"},
            {"Dash", "Dash"},
            {"Cover", "Cover"},
            {"Skill1", "Skill1"},
            {"Skill2", "Skill2"},
            {"Skill3", "Skill3"},
            {"Injector","Injector"},
            {"Grenade", "Grenade"},
            {"Map", "Map"},
            {"Pause", "Pause"}
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
}

void Coffee::RegisterInputBindings(sol::state& luaState)
{
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
        return Input::GetMousePosition();
    });

    inputTable.set_function("get_mouse_delta", []() {
        return Input::GetMouseDelta();
    });

    inputTable["MOUSE_MODE_NORMAL"] = false;
    inputTable["MOUSE_MODE_GRABBED"] = true;

    // It does not work with the property
    inputTable["mouse_mode"] = sol::property(
        [](bool grabbed) {
            Input::SetMouseGrabbed(grabbed);
        }
    );

    // Temporary solution to set the mouse mode
    inputTable.set_function("set_mouse_grabbed", [](bool grabbed) {
        Input::SetMouseGrabbed(grabbed);
    });

    inputTable.set_function("get_axis", [](const std::string& action) {
        return Input::GetBinding(action).AsAxis(false);
    });

    inputTable.set_function("get_direction",[](const std::string& action) {
        return Input::GetBinding(action).AsAxis(true);
    });

    inputTable.set_function("get_button", [](const std::string& action) {
        return Input::GetBinding(action).AsButton();
    });

    luaState["Input"] = inputTable;
}