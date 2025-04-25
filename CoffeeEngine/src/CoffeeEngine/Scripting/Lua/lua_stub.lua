-- Lua stub for CoffeeEngine module

-- Log functions
function log(message)
    -- Implementation here
end

function log_warning(message)
    -- Implementation here
end

function log_error(message)
    -- Implementation here
end

function log_critical(message)
    -- Implementation here
end

-- Input functions
input = {
    keycode = {
        UNKNOWN = 0,
        A = 1,
        B = 2,
        C = 3,
        D = 4,
        E = 5,
        F = 6,
        G = 7,
        H = 8,
        I = 9,
        J = 10,
        K = 11,
        L = 12,
        M = 13,
        N = 14,
        O = 15,
        P = 16,
        Q = 17,
        R = 18,
        S = 19,
        T = 20,
        U = 21,
        V = 22,
        W = 23,
        X = 24,
        Y = 25,
        Z = 26,
        D1 = 27,
        D2 = 28,
        D3 = 29,
        D4 = 30,
        D5 = 31,
        D6 = 32,
        D7 = 33,
        D8 = 34,
        D9 = 35,
        D0 = 36,
        RETURN = 37,
        ESCAPE = 38,
        BACKSPACE = 39,
        TAB = 40,
        SPACE = 41,
        MINUS = 42,
        EQUALS = 43,
        LEFTBRACKET = 44,
        RIGHTBRACKET = 45,
        BACKSLASH = 46,
        NONUSHASH = 47,
        SEMICOLON = 48,
        APOSTROPHE = 49,
        GRAVE = 50,
        COMMA = 51,
        PERIOD = 52,
        SLASH = 53,
        CAPSLOCK = 54,
        F1 = 55,
        F2 = 56,
        F3 = 57,
        F4 = 58,
        F5 = 59,
        F6 = 60,
        F7 = 61,
        F8 = 62,
        F9 = 63,
        F10 = 64,
        F11 = 65,
        F12 = 66,
        PRINTSCREEN = 67,
        SCROLLLOCK = 68,
        PAUSE = 69,
        INSERT = 70,
        HOME = 71,
        PAGEUP = 72,
        DELETE = 73,
        END = 74,
        PAGEDOWN = 75,
        RIGHT = 76,
        LEFT = 77,
        DOWN = 78,
        UP = 79,
        NUMLOCKCLEAR = 80,
        KP_DIVIDE = 81,
        KP_MULTIPLY = 82,
        KP_MINUS = 83,
        KP_PLUS = 84,
        KP_ENTER = 85,
        KP_1 = 86,
        KP_2 = 87,
        KP_3 = 88,
        KP_4 = 89,
        KP_5 = 90,
        KP_6 = 91,
        KP_7 = 92,
        KP_8 = 93,
        KP_9 = 94,
        KP_0 = 95,
        KP_PERIOD = 96,
        NONUSBACKSLASH = 97,
        APPLICATION = 98,
        POWER = 99,
        KP_EQUALS = 100,
        F13 = 101,
        F14 = 102,
        F15 = 103,
        F16 = 104,
        F17 = 105,
        F18 = 106,
        F19 = 107,
        F20 = 108,
        F21 = 109,
        F22 = 110,
        F23 = 111,
        F24 = 112,
        EXECUTE = 113,
        HELP = 114,
        MENU = 115,
        SELECT = 116,
        STOP = 117,
        AGAIN = 118,
        UNDO = 119,
        CUT = 120,
        COPY = 121,
        PASTE = 122,
        FIND = 123,
        MUTE = 124,
        VOLUMEUP = 125,
        VOLUMEDOWN = 126,
        KP_COMMA = 127,
        KP_EQUALSAS400 = 128,
        INTERNATIONAL1 = 129,
        INTERNATIONAL2 = 130,
        INTERNATIONAL3 = 131,
        INTERNATIONAL4 = 132,
        INTERNATIONAL5 = 133,
        INTERNATIONAL6 = 134,
        INTERNATIONAL7 = 135,
        INTERNATIONAL8 = 136,
        INTERNATIONAL9 = 137,
        LANG1 = 138,
        LANG2 = 139,
        LANG3 = 140,
        LANG4 = 141,
        LANG5 = 142,
        LANG6 = 143,
        LANG7 = 144,
        LANG8 = 145,
        LANG9 = 146,
        ALTERASE = 147,
        SYSREQ = 148,
        CANCEL = 149,
        CLEAR = 150,
        PRIOR = 151,
        RETURN2 = 152,
        SEPARATOR = 153,
        OUT = 154,
        OPER = 155,
        CLEARAGAIN = 156,
        CRSEL = 157,
        EXSEL = 158,
        KP_00 = 159,
        KP_000 = 160,
        THOUSANDSSEPARATOR = 161,
        DECIMALSEPARATOR = 162,
        CURRENCYUNIT = 163,
        CURRENCYSUBUNIT = 164,
        KP_LEFTPAREN = 165,
        KP_RIGHTPAREN = 166,
        KP_LEFTBRACE = 167,
        KP_RIGHTBRACE = 168,
        KP_TAB = 169,
        KP_BACKSPACE = 170,
        KP_A = 171,
        KP_B = 172,
        KP_C = 173,
        KP_D = 174,
        KP_E = 175,
        KP_F = 176,
        KP_XOR = 177,
        KP_POWER = 178,
        KP_PERCENT = 179,
        KP_LESS = 180,
        KP_GREATER = 181,
        KP_AMPERSAND = 182,
        KP_DBLAMPERSAND = 183,
        KP_VERTICALBAR = 184,
        KP_DBLVERTICALBAR = 185,
        KP_COLON = 186,
        KP_HASH = 187,
        KP_SPACE = 188,
        KP_AT = 189,
        KP_EXCLAM = 190,
        KP_MEMSTORE = 191,
        KP_MEMRECALL = 192,
        KP_MEMCLEAR = 193,
        KP_MEMADD = 194,
        KP_MEMSUBTRACT = 195,
        KP_MEMMULTIPLY = 196,
        KP_MEMDIVIDE = 197,
        KP_PLUSMINUS = 198,
        KP_CLEAR = 199,
        KP_CLEARENTRY = 200,
        KP_BINARY = 201,
        KP_OCTAL = 202,
        KP_DECIMAL = 203,
        KP_HEXADECIMAL = 204,
        LCTRL = 205,
        LSHIFT = 206,
        LALT = 207,
        LGUI = 208,
        RCTRL = 209,
        RSHIFT = 210,
        RALT = 211,
        RGUI = 212,
        MODE = 213,
        SLEEP = 214,
        WAKE = 215,
        CHANNEL_INCREMENT = 216,
        CHANNEL_DECREMENT = 217,
        MEDIA_PLAY = 218,
        MEDIA_PAUSE = 219,
        MEDIA_RECORD = 220,
        MEDIA_FAST_FORWARD = 221,
        MEDIA_REWIND = 222,
        MEDIA_NEXT_TRACK = 223,
        MEDIA_PREVIOUS_TRACK = 224,
        MEDIA_STOP = 225,
        MEDIA_EJECT = 226,
        MEDIA_PLAY_PAUSE = 227,
        MEDIA_SELECT = 228,
        AC_NEW = 229,
        AC_OPEN = 230,
        AC_CLOSE = 231,
        AC_EXIT = 232,
        AC_SAVE = 233,
        AC_PRINT = 234,
        AC_PROPERTIES = 235,
        AC_SEARCH = 236,
        AC_HOME = 237,
        AC_BACK = 238,
        AC_FORWARD = 239,
        AC_STOP = 240,
        AC_REFRESH = 241,
        AC_BOOKMARKS = 242,
        SOFTLEFT = 243,
        SOFTRIGHT = 244,
        CALL = 245,
        ENDCALL = 246
    },
    mousecode = {
        Left = 0,
        Middle = 1,
        Right = 2,
        X1 = 3,
        X2 = 4
    },
    action = {
    UiMoveHorizontal = 0,
    UiMoveVertical = 1,
    Confirm = 2,
    Cancel = 3,
    MoveHorizontal = 4,
    MoveVertical = 5,
    AimHorizontal = 6,
    AimVertical = 7,
    Shoot = 8,
    Melee = 9,
    Interact = 10,
    Dash = 11,
    Cover = 12,
    Skill1 = 13,
    Skill2 = 14,
    Skill3 = 15,
    Injector = 16,
    Grenade = 17,
    Map = 18,
    Pause = 19,
    },
    state = {
        Idle = 0,
        Up = 1,
        Down = 2,
        Repeat = 3,
    },
    is_key_pressed = function(key)
        -- Implementation here
        return false
    end,
    is_mouse_button_pressed = function(button)
        -- Implementation here
        return false
    end,
    is_button_pressed = function(action)
        -- Implementation here
        return 0
    end,
    get_axis_position = function(action)
        -- Implementation here
        return 0.0
    end,
    get_mouse_position = function()
        -- Implementation here
        return 0, 0
    end,
    get_axis = function(action)
        -- Implementation here
        return 0.0
    end,
    get_direction = function(action)
        -- Implementation here
        return 0
    end,
    get_button = function(action)
        -- Implementation here
        return 0
    end
    send_rumble = function(left_power, right_power, duration)
        -- Implementation here
        return
    end
}

-- Timer functions
Stopwatch = {
    start = function(self) end,
    stop = function(self) end,
    reset = function(self) end,
    get_elapsed_time = function(self) return 0 end,
    get_precise_elapsed_time = function(self) return 0.0 end
}

Timer = {
    start = function(self) end,
    stop = function(self) end,
    set_wait_time = function(self, t) end,
    get_wait_time = function(self) return 0.0 end,
    set_one_shot = function(self, val) end,
    is_one_shot = function(self) return false end,
    set_auto_start = function(self, val) end,
    is_auto_start = function(self) return false end,
    set_paused = function(self, val) end,
    is_paused = function(self) return false end,
    get_time_left = function(self) return 0.0 end,
    set_callback = function(self, fn) end
}

create_timer = function(waitTime, autoStart, oneShot, callback)
    return Timer
end

-- Component stubs
TagComponent = {
    Tag = ""
}

TransformComponent = {
    Position = {0.0, 0.0, 0.0},
    Rotation = {0.0, 0.0, 0.0},
    Scale = {1.0, 1.0, 1.0},
    get_local_transform = function()
        -- Implementation here
        return {}
    end,
    set_local_transform = function(transform)
        -- Implementation here
    end,
    get_world_transform = function()
        -- Implementation here
        return {}
    end,
    set_world_transform = function(transform)
        -- Implementation here
    end
}

CameraComponent = {
    Camera = {}
}

MeshComponent = {
    mesh = {},
    drawAABB = false,
    get_mesh = function()
        -- Implementation here
        return {}
    end
}

MaterialComponent = {
    material = {}
}

LightComponent = {
    Color = {1.0, 1.0, 1.0},
    Direction = {0.0, -1.0, 0.0},
    Position = {0.0, 0.0, 0.0},
    Range = 5.0,
    Attenuation = 1.0,
    Intensity = 1.0,
    Angle = 45.0,
    type = 0
}

-- Entity functions
Entity = {
    add_component = function(self, componentName)
        -- Implementation here
    end,
    get_component = function(self, componentName)
        -- Implementation here
        return {}
    end,
    has_component = function(self, componentName)
        -- Implementation here
        return false
    end,
    remove_component = function(self, componentName)
        -- Implementation here
    end,
    set_parent = function(self, parent)
        -- Implementation here
    end,
    is_valid = function(self)
        -- Implementation here
        return true
    end
}

-- UI Components

UIAnchorPosition = {
    TopLeft = 0,
    TopCenter = 1,
    TopRight = 2,
    CenterLeft = 3,
    Center = 4,
    CenterRight = 5,
    BottomLeft = 6,
    BottomCenter = 7,
    BottomRight = 8
}

UIImageComponent = {
    Size = {0.0, 0.0},
    Visible = true,
    Layer = 0,
    set_texture = function(self, texture)
    end
}

UITextComponent = {
    Text = "",
    FontPath = "",
    FontSize = 12,
    Color = {1.0, 1.0, 1.0, 1.0},
    Layer = 0,
    Visible = true
}

UIButtonComponent = {
    Visible = true,
    BaseTexture = "",
    SelectedTexture = "",
    PressedTexture = "",
    BaseSize = {0.0, 0.0},
    SelectedSize = {0.0, 0.0},
    PressedSize = {0.0, 0.0},
    BaseColor = {1.0, 1.0, 1.0, 1.0},
    SelectedColor = {1.0, 1.0, 1.0, 1.0},
    PressedColor = {1.0, 1.0, 1.0, 1.0},
    Layer = 0,
    set_state = function(self, state)
    end,
    get_state = function(self)
        return "Base"
    end
}

UISliderComponent = {
    BarTexture = "",
    HandleTexture = "",
    Size = {0.0, 0.0},
    HandleSize = {0.0, 0.0},
    Value = 0.0,
    Visible = true,
    Layer = 0,
    on_value_changed = function(self, callback)
    end
}

UIToggleComponent = {
    IsActive = false,
    Visible = true,
    Size = {0.0, 0.0},
    Layer = 0,
    ActiveTexture = "",
    InactiveTexture = "",
    set_active = function(self, active)
    end,
    toggle = function(self)
    end
}

-- Particles Component

ParticlesSystemComponent = {
    emit = function(self)
        -- Implementation here
    end,
    set_looping = function(self, value)
        -- Implementation here
    end,
    get_emitter = function(self)
        return {}
    end
}

-- Physics Component

RigidBodyType = {
    Static = 0,
    Dynamic = 1,
    Kinematic = 2
}

RigidBodyProperties = {
    type = RigidBodyType.Static,
    mass = 1.0,
    useGravity = true,
    freezeX = false,
    freezeY = false,
    freezeZ = false,
    freezeRotX = false,
    freezeRotY = false,
    freezeRotZ = false,
    isTrigger = false,
    velocity = {0.0, 0.0, 0.0},
    friction = 0.5,
    linearDrag = 0.0,
    angularDrag = 0.0
}

RigidBody = {
    set_position = function(self, pos) end,
    get_position = function(self) return {0.0, 0.0, 0.0} end,
    set_rotation = function(self, rot) end,
    get_rotation = function(self) return {0.0, 0.0, 0.0} end,
    set_velocity = function(self, vel) end,
    get_velocity = function(self) return {0.0, 0.0, 0.0} end,
    add_velocity = function(self, vel) end,
    apply_force = function(self, force) end,
    apply_impulse = function(self, impulse) end,
    reset_velocity = function(self) end,
    clear_forces = function(self) end,
    apply_torque = function(self, torque) end,
    apply_torque_impulse = function(self, impulse) end,
    set_angular_velocity = function(self, vel) end,
    get_angular_velocity = function(self) return {0.0, 0.0, 0.0} end,
    set_trigger = function(self, is_trigger) end,
    get_body_type = function(self) return RigidBodyType.Static end,
    set_body_type = function(self, body_type) end,
    get_mass = function(self) return 1.0 end,
    set_mass = function(self, mass) end,
    get_use_gravity = function(self) return true end,
    set_use_gravity = function(self, use) end,
    get_freeze_x = function(self) return false end,
    set_freeze_x = function(self, val) end,
    get_freeze_y = function(self) return false end,
    set_freeze_y = function(self, val) end,
    get_freeze_z = function(self) return false end,
    set_freeze_z = function(self, val) end,
    get_freeze_rot_x = function(self) return false end,
    set_freeze_rot_x = function(self, val) end,
    get_freeze_rot_y = function(self) return false end,
    set_freeze_rot_y = function(self, val) end,
    get_freeze_rot_z = function(self) return false end,
    set_freeze_rot_z = function(self, val) end,
    get_friction = function(self) return 0.5 end,
    set_friction = function(self, val) end,
    get_linear_drag = function(self) return 0.0 end,
    set_linear_drag = function(self, val) end,
    get_angular_drag = function(self) return 0.0 end,
    set_angular_drag = function(self, val) end,
    get_is_trigger = function(self) return false end
}

BoxCollider = {
    new = function(size) return {} end
}
SphereCollider = {
    new = function(radius) return {} end
}
CapsuleCollider = {
    new = function(radius, height) return {} end
}

Physics = {
    raycast = function(origin, direction, maxDistance)
        return {
            hasHit = false,
            hitEntity = nil,
            hitPoint = {0.0, 0.0, 0.0},
            hitNormal = {0.0, 1.0, 0.0},
            hitFraction = 0.0
        }
    end,
    raycast_all = function(origin, direction, maxDistance)
        return {}
    end,
    raycast_any = function(origin, direction, maxDistance)
        return false
    end,
    debug_draw_raycast = function(origin, direction, maxDistance, rayColor, hitColor)
        -- Implementation here
    end
}

create_box_collider = function(size)
    return BoxCollider.new(size)
end

create_sphere_collider = function(radius)
    return SphereCollider.new(radius)
end

create_capsule_collider = function(radius, height)
    return CapsuleCollider.new(radius, height)
end

create_rigidbody = function(properties, collider)
    return RigidBody
end

-- Audio Component

AudioSourceComponent = {
    set_volume = function(self, volume)
        -- Implementation here
    end,
    play = function(self)
        -- Implementation here
    end,
    pause = function(self)
        -- Implementation here
    end
}

-- Animator Component

AnimatorComponent = {
    set_current_animation = function(self, animationName)
        -- Implementation here
    end,
    set_upper_animation = function(self, animationName)
        -- Implementation here
    end,
    set_lower_animation = function(self, animationName)
        -- Implementation here
    end
}


-- Scene Management

Scene = {
    create_entity = function(self) return Entity end,
    destroy_entity = function(self, entity) end,
    duplicate_entity = function(self, entity) return Entity end,
    get_entity_by_name = function(self, name) return Entity end,
    get_all_entities = function(self) return {} end
}

SceneManager = {
    preload_scene = function(path) end,
    preload_scene_async = function(path) end,
    change_scene = function(path_or_scene) end,
    change_scene_async = function(path) end
}

