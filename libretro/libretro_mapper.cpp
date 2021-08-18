// This is copyrighted software. More information is at the end of this file.
#include "CoreOptions.h"
#include "dosbox.h"
#include "joystick.h"
#include "keyboard.h"
#include "libretro.h"
#include "libretro_dosbox.h"
#include "libretro-vkbd.h"
#include "log.h"
#include "mapper.h"
#include "mouse.h"
#include <cmath>
#include <memory>
#include <tuple>
#include <vector>

#define RDEV(x) RETRO_DEVICE_##x
#define RDIX(x) RETRO_DEVICE_INDEX_##x
#define RDID(x) RETRO_DEVICE_ID_##x

class Processable;
static std::vector<std::unique_ptr<Processable>> input_list;

static bool keyboard_state[KBD_LAST]{false};
static bool use_slow_mouse = false;
static bool use_fast_mouse = false;

static constexpr std::tuple<retro_key, KBD_KEYS> retro_dosbox_map[]{
    {RETROK_1, KBD_1},
    {RETROK_2, KBD_2},
    {RETROK_3, KBD_3},
    {RETROK_4, KBD_4},
    {RETROK_5, KBD_5},
    {RETROK_6, KBD_6},
    {RETROK_7, KBD_7},
    {RETROK_8, KBD_8},
    {RETROK_9, KBD_9},
    {RETROK_0, KBD_0},
    {RETROK_a, KBD_a},
    {RETROK_b, KBD_b},
    {RETROK_c, KBD_c},
    {RETROK_d, KBD_d},
    {RETROK_e, KBD_e},
    {RETROK_f, KBD_f},
    {RETROK_g, KBD_g},
    {RETROK_h, KBD_h},
    {RETROK_i, KBD_i},
    {RETROK_j, KBD_j},
    {RETROK_k, KBD_k},
    {RETROK_l, KBD_l},
    {RETROK_m, KBD_m},
    {RETROK_n, KBD_n},
    {RETROK_o, KBD_o},
    {RETROK_p, KBD_p},
    {RETROK_q, KBD_q},
    {RETROK_r, KBD_r},
    {RETROK_s, KBD_s},
    {RETROK_t, KBD_t},
    {RETROK_u, KBD_u},
    {RETROK_v, KBD_v},
    {RETROK_w, KBD_w},
    {RETROK_x, KBD_x},
    {RETROK_y, KBD_y},
    {RETROK_z, KBD_z},
    {RETROK_F1, KBD_f1},
    {RETROK_F2, KBD_f2},
    {RETROK_F3, KBD_f3},
    {RETROK_F4, KBD_f4},
    {RETROK_F5, KBD_f5},
    {RETROK_F6, KBD_f6},
    {RETROK_F7, KBD_f7},
    {RETROK_F8, KBD_f8},
    {RETROK_F9, KBD_f9},
    {RETROK_F10, KBD_f10},
    {RETROK_F11, KBD_f11},
    {RETROK_F12, KBD_f12},
    {RETROK_ESCAPE, KBD_esc},
    {RETROK_TAB, KBD_tab},
    {RETROK_BACKSPACE, KBD_backspace},
    {RETROK_RETURN, KBD_enter},
    {RETROK_SPACE, KBD_space},
    {RETROK_LALT, KBD_leftalt},
    {RETROK_RALT, KBD_rightalt},
    {RETROK_LCTRL, KBD_leftctrl},
    {RETROK_RCTRL, KBD_rightctrl},
    {RETROK_LSHIFT, KBD_leftshift},
    {RETROK_RSHIFT, KBD_rightshift},
    {RETROK_CAPSLOCK, KBD_capslock},
    {RETROK_SCROLLOCK, KBD_scrolllock},
    {RETROK_NUMLOCK, KBD_numlock},
    {RETROK_MINUS, KBD_minus},
    {RETROK_EQUALS, KBD_equals},
    {RETROK_BACKSLASH, KBD_backslash},
    {RETROK_LEFTBRACKET, KBD_leftbracket},
    {RETROK_RIGHTBRACKET, KBD_rightbracket},
    {RETROK_SEMICOLON, KBD_semicolon},
    {RETROK_QUOTE, KBD_quote},
    {RETROK_PERIOD, KBD_period},
    {RETROK_COMMA, KBD_comma},
    {RETROK_SLASH, KBD_slash},
    {RETROK_SYSREQ, KBD_printscreen},
    {RETROK_PAUSE, KBD_pause},
    {RETROK_INSERT, KBD_insert},
    {RETROK_HOME, KBD_home},
    {RETROK_PAGEUP, KBD_pageup},
    {RETROK_PAGEDOWN, KBD_pagedown},
    {RETROK_DELETE, KBD_delete},
    {RETROK_END, KBD_end},
    {RETROK_LEFT, KBD_left},
    {RETROK_UP, KBD_up},
    {RETROK_DOWN, KBD_down},
    {RETROK_RIGHT, KBD_right},
    {RETROK_KP1, KBD_kp1},
    {RETROK_KP2, KBD_kp2},
    {RETROK_KP3, KBD_kp3},
    {RETROK_KP4, KBD_kp4},
    {RETROK_KP5, KBD_kp5},
    {RETROK_KP6, KBD_kp6},
    {RETROK_KP7, KBD_kp7},
    {RETROK_KP8, KBD_kp8},
    {RETROK_KP9, KBD_kp9},
    {RETROK_KP0, KBD_kp0},
    {RETROK_KP_DIVIDE, KBD_kpdivide},
    {RETROK_KP_MULTIPLY, KBD_kpmultiply},
    {RETROK_KP_MINUS, KBD_kpminus},
    {RETROK_KP_PLUS, KBD_kpplus},
    {RETROK_KP_ENTER, KBD_kpenter},
    {RETROK_KP_PERIOD, KBD_kpperiod},
    {RETROK_BACKQUOTE, KBD_grave},
    {RETROK_OEM_102, KBD_extra_lt_gt},
};

static constexpr KBD_KEYS event_key_map[]{
    KBD_f1,    KBD_f2,      KBD_f3,         KBD_f4,          KBD_f5,    KBD_f6,
    KBD_f7,    KBD_f8,      KBD_f9,         KBD_f10,         KBD_f11,   KBD_f12,
    KBD_enter, KBD_kpminus, KBD_scrolllock, KBD_printscreen, KBD_pause, KBD_home,
};

static constexpr unsigned eventMOD1 = 55;
static constexpr unsigned eventMOD2 = 53;

template <class T>
class InputItem final
{
public:
    void process(const T& item, const bool is_down)
    {
        if (is_down && !is_down_) {
            item.press();
        } else if (!is_down && is_down_) {
            item.release();
        }
        is_down_ = is_down;
    }

private:
    bool is_down_ = false;
};

class Processable
{
public:
    virtual void process() = 0;
    virtual ~Processable() = default;
};

class EventHandler final: public Processable
{
public:
    EventHandler(MAPPER_Handler* const handler, const MapKeys key, const unsigned mods)
        : handler_(handler)
        , key_(event_key_map[key])
        , mods_(mods)
    { }

    void process() override
    {
        const uint32_t modsList =
            keyboard_state[eventMOD1] ? 1 : 0 | keyboard_state[eventMOD2] ? 1 : 0;
        item.process(*this, (mods_ == modsList) && keyboard_state[key_]);
    }

    void press() const
    {
        if (retro_vkbd) {
            return;
        }
        handler_(true);
    }

    void release() const
    {
        handler_(false);
    }

private:
    MAPPER_Handler* handler_;
    unsigned key_;
    unsigned mods_;
    InputItem<EventHandler> item;
};

class VKBDToggle final: public Processable
{
public:
    VKBDToggle(const unsigned retro_port, const unsigned retro_id)
        : retro_port_(retro_port)
        , retro_id_(retro_id)
    { }

    void process() override
    {
        item_.process(*this, joypad_bits[retro_port_] & (1 << retro_id_));
    }

    void press() const
    {
        toggle_vkbd();
    }

    void release() const
    { }

private:
    unsigned retro_port_;
    unsigned retro_id_;
    InputItem<VKBDToggle> item_;
};

class MouseButton final: public Processable
{
public:
    MouseButton(const unsigned retro_button, const Bit8u dosbox_button)
        : retro_button_(retro_button)
        , dosbox_button_(dosbox_button)
    { }

    void process() override
    {
        item_.process(*this, input_cb(0, RDEV(MOUSE), 0, retro_button_));
    }

    void press() const
    {
        if (retro_vkbd) {
            return;
        }
        Mouse_ButtonPressed(dosbox_button_);
    }

    void release() const
    {
        Mouse_ButtonReleased(dosbox_button_);
    }

private:
    unsigned retro_button_;
    Bit8u dosbox_button_;
    InputItem<MouseButton> item_;
};

class EmulatedMouseButton final: public Processable
{
public:
    EmulatedMouseButton(
        const unsigned retro_port, const unsigned retro_id, const Bit8u dosbox_button)
        : retro_port_(retro_port)
        , retro_id_(retro_id)
        , dosbox_button_(dosbox_button)
    { }

    void process() override
    {
        item_.process(*this, joypad_bits[retro_port_] & (1 << retro_id_));
    }

    void press() const
    {
        if (retro_vkbd) {
            return;
        }
        if (dosbox_button_ == 2) {
            use_slow_mouse = true;
        } else if (dosbox_button_ == 3) {
            use_fast_mouse = true;
        } else {
            Mouse_ButtonPressed(dosbox_button_);
        }
    }

    void release() const
    {
        if (dosbox_button_ == 2) {
            use_slow_mouse = false;
        } else if (dosbox_button_ == 3) {
            use_fast_mouse = false;
        } else {
            Mouse_ButtonReleased(dosbox_button_);
        }
    }

private:
    unsigned retro_port_;
    unsigned retro_id_;
    Bit8u dosbox_button_;
    InputItem<EmulatedMouseButton> item_;
};

class JoystickButton final: public Processable
{
public:
    JoystickButton(
        const unsigned retro_port, const unsigned retro_id, const unsigned dosbox_port,
        const unsigned dosbox_id)
        : retro_port_(retro_port)
        , retro_id_(retro_id)
        , dosbox_port_(dosbox_port)
        , dosbox_id_(dosbox_id)
    { }

    void process() override
    {
        item_.process(*this, joypad_bits[retro_port_] & (1 << retro_id_));
    }

    void press() const
    {
        if (retro_vkbd) {
            return;
        }
        JOYSTICK_Button(dosbox_port_, dosbox_id_ & 1, true);
    }

    void release() const
    {
        JOYSTICK_Button(dosbox_port_, dosbox_id_ & 1, false);
    }

private:
    unsigned retro_port_;
    unsigned retro_id_;
    unsigned dosbox_port_;
    unsigned dosbox_id_;
    InputItem<JoystickButton> item_;
};

class JoystickAxis final: public Processable
{
public:
    JoystickAxis(
        const unsigned retro_port, const unsigned retro_side, const unsigned retro_axis,
        const unsigned dosbox_port, const unsigned dosbox_axis)
        : retro_port_(retro_port)
        , retro_side_(retro_side)
        , retro_axis_(retro_axis)
        , dosbox_port_(dosbox_port)
        , dosbox_axis_(dosbox_axis)
    { }

    void process() override
    {
        const float value = input_cb(retro_port_, RDEV(ANALOG), retro_side_, retro_axis_);
        if (dosbox_axis_ == 0) {
            JOYSTICK_Move_X(dosbox_port_, value / 32768.0f);
        } else {
            JOYSTICK_Move_Y(dosbox_port_, value / 32768.0f);
        }
    }

private:
    unsigned retro_port_;
    unsigned retro_side_;
    unsigned retro_axis_;
    unsigned dosbox_port_;
    unsigned dosbox_axis_;
};

class JoystickHat final: public Processable
{
public:
    JoystickHat(
        const unsigned retro_port, const unsigned retro_id, const unsigned dosbox_port,
        const unsigned dosbox_axis)
        : retro_port_(retro_port)
        , retro_id_(retro_id)
        , dosbox_port_(dosbox_port)
        , dosbox_axis_(dosbox_axis)
    { }

    void process() override
    {
        item_.process(*this, joypad_bits[retro_port_] & (1 << retro_id_));
    }

    void press() const
    {
        if (retro_vkbd) {
            return;
        }
        if (dosbox_axis_ == 0) {
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_LEFT) {
                JOYSTICK_Move_X(dosbox_port_, -1.0f);
            }
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_RIGHT) {
                JOYSTICK_Move_X(dosbox_port_, 1.0f);
            }
        } else {
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_UP) {
                JOYSTICK_Move_Y(dosbox_port_, -1);
            }
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_DOWN) {
                JOYSTICK_Move_Y(dosbox_port_, 1);
            }
        }
    }

    void release() const
    {
        if (dosbox_axis_ == 0) {
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_LEFT) {
                JOYSTICK_Move_X(dosbox_port_, -0);
            }
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_RIGHT) {
                JOYSTICK_Move_X(dosbox_port_, 0);
            }
        } else {
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_UP) {
                JOYSTICK_Move_Y(dosbox_port_, -0);
            }
            if (retro_id_ == RETRO_DEVICE_ID_JOYPAD_DOWN) {
                JOYSTICK_Move_Y(dosbox_port_, 0);
            }
        }
    }

private:
    unsigned retro_port_;
    unsigned retro_id_;
    unsigned dosbox_port_;
    unsigned dosbox_axis_;
    InputItem<JoystickHat> item_;
};

void retro_key_down(const int keycode)
{
    for (const auto [retro_id, dosbox_id] : retro_dosbox_map) {
        if (retro_id == keycode) {
            KEYBOARD_AddKey(dosbox_id, 1);
            return;
        }
    }
}

void retro_key_up(const int keycode)
{
    for (const auto [retro_id, dosbox_id] : retro_dosbox_map) {
        if (retro_id == keycode) {
            KEYBOARD_AddKey(dosbox_id, 0);
            return;
        }
    }
}

static RETRO_CALLCONV void keyboardEventCb(
    const bool down, const unsigned keycode, const uint32_t /*character*/,
    const uint16_t /*key_modifiers*/)
{
    if (retro_vkbd) {
        if (keycode == RETROK_CAPSLOCK) {
            if (down && !keyboard_state[KBD_capslock]) {
                KEYBOARD_AddKey(KBD_capslock, down);
                retro_capslock = !retro_capslock;
            } else if (!down && keyboard_state[KBD_capslock]) {
                KEYBOARD_AddKey(KBD_capslock, down);
            }
            keyboard_state[KBD_capslock] = down;
            return;
        } else if (down) {
            return;
        }
    }

    for (const auto [retro_id, dosbox_id] : retro_dosbox_map) {
        if (retro_id == keycode) {
            if (keyboard_state[dosbox_id] != down) {
                keyboard_state[dosbox_id] = down;
                KEYBOARD_AddKey(dosbox_id, down);
                if (keycode == RETROK_CAPSLOCK) {
                    retro_capslock = down;
                }
            }
            return;
        }
    }
}

static constexpr auto make2buttonGamepadDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 6>
{
    return {{
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
    }};
}

static constexpr auto make2buttonJoystickDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 4>
{
    return {{
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,
         "Left Analog X"},
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,
         "Left Analog Y"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
    }};
}

static constexpr auto make4buttonGamepadDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 8>
{
    return {{
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "D-Pad Left"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "D-Pad Up"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "D-Pad Down"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Button 4"},
    }};
}

static constexpr auto make4buttonJoystickDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 8>
{
    return {{
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X,
         "Left Analog X"},
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y,
         "Left Analog Y"},
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,
         "Right Analog X"},
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,
         "Right Analog Y"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Button 1"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Button 2"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Button 3"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Button 4"},
    }};
}

static constexpr auto makeEmulatedMouseDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 6>
{
    return {{
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X,
         "Emulated Mouse X Axis"},
        {port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y,
         "Emulated Mouse Y Axis"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "Emulated Mouse Left Click"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "Emulated Mouse Right Click"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "Emulated Mouse Slow Down"},
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Emulated Mouse Speed Up"},
    }};
}

static constexpr auto makeVKBDDescArray(const unsigned int port)
    -> std::array<retro_input_descriptor, 1>
{
    return {{
        {port, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Toggle Virtual Keyboard"},
    }};
}

static void addDpad(const unsigned int retro_port, const unsigned int dos_port)
{
    input_list.push_back(std::make_unique<JoystickHat>(retro_port, RDID(JOYPAD_LEFT), dos_port, 0));
    input_list.push_back(
        std::make_unique<JoystickHat>(retro_port, RDID(JOYPAD_RIGHT), dos_port, 0));
    input_list.push_back(std::make_unique<JoystickHat>(retro_port, RDID(JOYPAD_UP), dos_port, 1));
    input_list.push_back(std::make_unique<JoystickHat>(retro_port, RDID(JOYPAD_DOWN), dos_port, 1));
};

static void add2Buttons(const unsigned int retro_port, const unsigned int dos_port)
{
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_B), dos_port, 0));
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_A), dos_port, 1));
};

static void add4Buttons(const unsigned int retro_port)
{
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_B), 0, 0));
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_A), 0, 1));
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_Y), 1, 0));
    input_list.push_back(std::make_unique<JoystickButton>(retro_port, RDID(JOYPAD_X), 1, 1));
};

static void add2Axes(const unsigned int retro_port, const unsigned int dos_port)
{
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_LEFT), RDID(ANALOG_X), dos_port, 0));
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_LEFT), RDID(ANALOG_Y), dos_port, 1));
};

static void add4Axes(const unsigned int retro_port)
{
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_LEFT), RDID(ANALOG_X), 0, 0));
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_LEFT), RDID(ANALOG_Y), 0, 1));
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_RIGHT), RDID(ANALOG_X), 1, 0));
    input_list.push_back(
        std::make_unique<JoystickAxis>(retro_port, RDIX(ANALOG_RIGHT), RDID(ANALOG_Y), 1, 1));
};

static void addMouseEmulationButtons(const unsigned int retro_port)
{
    input_list.push_back(std::make_unique<EmulatedMouseButton>(retro_port, RDID(JOYPAD_R), 0));
    input_list.push_back(std::make_unique<EmulatedMouseButton>(retro_port, RDID(JOYPAD_L), 1));
    input_list.push_back(std::make_unique<EmulatedMouseButton>(retro_port, RDID(JOYPAD_R2), 2));
    input_list.push_back(std::make_unique<EmulatedMouseButton>(retro_port, RDID(JOYPAD_L2), 3));
}

static void addVKBDButton(const unsigned int retro_port)
{
    input_list.push_back(std::make_unique<VKBDToggle>(retro_port, RDID(JOYPAD_SELECT)));
}

static auto get_active_ports() noexcept -> std::tuple<int, int, int>
{
    int active_port_count = 0;
    int first_retro_port = 0;
    int second_retro_port = 0;

    for (int retro_port = 0;
         active_port_count < 2 && retro_port < static_cast<int>(connected.size()); ++retro_port)
    {
        if (connected[retro_port]) {
            ++active_port_count;
            if (active_port_count == 1) {
                first_retro_port = retro_port;
            } else if (active_port_count == 2) {
                second_retro_port = retro_port;
            }
        }
    }
    return {active_port_count, first_retro_port, second_retro_port};
}

void MAPPER_Init()
{
    retro_keyboard_callback callback{keyboardEventCb};
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &callback);

    input_list.clear();
    input_list.push_back(std::make_unique<MouseButton>(RDID(MOUSE_LEFT), 0));
    input_list.push_back(std::make_unique<MouseButton>(RDID(MOUSE_RIGHT), 1));
    input_list.push_back(std::make_unique<MouseButton>(RDID(MOUSE_MIDDLE), 2));

    // Currently unused.
#if 0
    constexpr retro_input_descriptor desc_kbd[]{
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Kbd Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Kbd Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Kbd Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Kbd Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Enter" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Esc" },
    };
#endif

    std::vector<retro_input_descriptor> retro_desc;

    auto addToRetroDesc = [&](const auto& input_desc_list) {
        for (const auto& desc : input_desc_list) {
            retro_desc.push_back(desc);
            retro::logDebug("Map: {}.", desc.description);
        }
    };

    JOYSTICK_Enable(0, true);
    JOYSTICK_Enable(1, true);

    const auto [active_port_count, first_retro_port, second_retro_port] = get_active_ports();

    if (active_port_count == 2) {
        int dos_port = 0;
        retro::logDebug("Both ports connected, deferring to two axis, two button pads.");
        update_dosbox_variable(false, "joystick", "joysticktype", "2axis");
        ::joytype = JOY_2AXIS;
        for (const auto retro_port : {first_retro_port, second_retro_port}) {
            retro::logDebug("Port {} connected.", retro_port);
            if (gamepad[retro_port]) {
                retro::logDebug("Port {} gamepad.", retro_port);
                addDpad(retro_port, dos_port);
                addToRetroDesc(make2buttonGamepadDescArray(retro_port));
            } else {
                retro::logDebug("Port {} joystick.", retro_port);
                add2Axes(retro_port, dos_port);
                addToRetroDesc(make2buttonJoystickDescArray(retro_port));
            }
            add2Buttons(retro_port, dos_port);
            if (emulated_mouse) {
                addMouseEmulationButtons(retro_port);
                addToRetroDesc(makeEmulatedMouseDescArray(retro_port));
            }
            ++dos_port;
        }
    } else if (active_port_count == 1) {
        const bool force_2axis = retro::core_options["joystick_force_2axis"].toBool();
        if (force_2axis) {
            retro::logDebug(
                "One port connected, but enabling only 2axis in connected port as forced in core "
                "options.");
            ::joytype = JOY_2AXIS;
            update_dosbox_variable(false, "joystick", "joysticktype", "2axis");
        } else {
            retro::logDebug("One port connected, enabling 4 buttons in connected port.");
            ::joytype = JOY_4AXIS;
            update_dosbox_variable(false, "joystick", "joysticktype", "4axis");
        }
        retro::logDebug("Port {} connected.", first_retro_port);
        if (gamepad[first_retro_port]) {
            retro::logDebug("Port {} gamepad.", first_retro_port);
            addDpad(first_retro_port, 0);
            if (force_2axis) {
                addToRetroDesc(make2buttonGamepadDescArray(first_retro_port));
            } else {
                addToRetroDesc(make4buttonGamepadDescArray(first_retro_port));
            }
        } else {
            retro::logDebug("Port {} joystick.", first_retro_port);
            if (force_2axis) {
                add2Axes(first_retro_port, 0);
                addToRetroDesc(make2buttonJoystickDescArray(first_retro_port));
            } else {
                add4Axes(first_retro_port);
                addToRetroDesc(make4buttonJoystickDescArray(first_retro_port));
            }
        }
        if (force_2axis) {
            add2Buttons(first_retro_port, 0);
        } else {
            add4Buttons(first_retro_port);
        }
        if (emulated_mouse) {
            addMouseEmulationButtons(first_retro_port);
            addToRetroDesc(makeEmulatedMouseDescArray(first_retro_port));
        }
    } else {
        update_dosbox_variable(false, "joystick", "joysticktype", "none");
        JOYSTICK_Enable(0, false);
        JOYSTICK_Enable(1, false);
    }

    addVKBDButton(0);
    addToRetroDesc(makeVKBDDescArray(0));
    addVKBDButton(1);
    addToRetroDesc(makeVKBDDescArray(1));

    retro_desc.push_back({}); // null terminator
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, retro_desc.data());
}

void MAPPER_AddHandler(
    MAPPER_Handler* const handler, const MapKeys key, const Bitu mods,
    const char* const /*eventname*/, const char* const /*buttonname*/)
{
    input_list.push_back(std::make_unique<EventHandler>(handler, key, mods));
}

static void runMouseEmulation(const unsigned int port)
{
    int16_t emulated_mouse_x = input_cb(
        port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X);
    int16_t emulated_mouse_y = input_cb(
        port, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y);

    const float deadzone = mouse_emu_deadzone * (32768.0f / 100.0f);
    const float magnitude =
        sqrtf((emulated_mouse_x * emulated_mouse_x) + (emulated_mouse_y * emulated_mouse_y));
    if (magnitude <= deadzone) {
        emulated_mouse_x = 0;
        emulated_mouse_y = 0;
    }

    float slowdown = 32768.0;
    if (use_fast_mouse) {
        slowdown /= 3.0f;
    }
    if (use_slow_mouse) {
        slowdown *= 8.0f;
    }

    const float adjusted_x = emulated_mouse_x * mouse_speed_factor_x * 8.0f / slowdown;
    const float adjusted_y = emulated_mouse_y * mouse_speed_factor_y * 8.0f / slowdown;
    Mouse_CursorMoved(adjusted_x, adjusted_y, 0, 0, true);
}

void MAPPER_Run(const bool /*pressed*/)
{
    poll_cb();

    for (unsigned j = 0; j < RETRO_DEVICES; ++j) {
        if (libretro_supports_bitmasks) {
            joypad_bits[j] = input_cb(j, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
        } else {
            joypad_bits[j] = 0;
            for (unsigned i = 0; i < RETRO_DEVICE_ID_JOYPAD_R3 + 1; ++i) {
                joypad_bits[j] |= input_cb(j, RETRO_DEVICE_JOYPAD, 0, i) ? (1 << i) : 0;
            }
        }
    }

    // Virtual keyboard for ports 1 & 2.
    input_vkbd();

    if (emulated_mouse) {
        if (connected[0]) {
            runMouseEmulation(0);
        }
        if (connected[1]) {
            runMouseEmulation(1);
        }
    }

    int16_t mouse_x = input_cb(0, RDEV(MOUSE), 0, RDID(MOUSE_X));
    int16_t mouse_y = input_cb(0, RDEV(MOUSE), 0, RDID(MOUSE_Y));
    if (mouse_x || mouse_y) {
        float slowdown = 1.0;
        if (use_fast_mouse) {
            slowdown /= 3.0f;
        }
        if (use_slow_mouse) {
            slowdown *= 8.0f;
        }
        float adjusted_x = mouse_x * mouse_speed_factor_x / slowdown;
        float adjusted_y = mouse_y * mouse_speed_factor_y / slowdown;

        if (!retro_vkbd) {
            Mouse_CursorMoved(adjusted_x, adjusted_y, 0, 0, true);
        }
    }

    for (const auto& processable : input_list) {
        processable->process();
    }
}

void Mouse_AutoLock(const bool /*enable*/)
{ }

/*

Copyright (C) 2015-2019 Andrés Suárez
Copyright (C) 2020 Nikos Chantziaras <realnc@gmail.com>

This file is part of DOSBox-core.

DOSBox-core is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 2 of the License, or (at your option) any later
version.

DOSBox-core is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
DOSBox-core. If not, see <https://www.gnu.org/licenses/>.

*/
