#pragma once
#include "config.h"
#include "keyboard.h"
#include "libretro.h"
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <set>
#include <vector>

#define RETRO_DEVICES 5
constexpr int GFX_MAX_WIDTH = 1280;
constexpr int GFX_MAX_HEIGHT = 1024;

extern retro_input_poll_t poll_cb;
extern retro_input_state_t input_cb;
extern retro_environment_t environ_cb;
extern bool run_synced;
extern float dosbox_aspect_ratio;
extern std::array<std::vector<Bit8u>, 2> dosbox_framebuffers;
extern std::vector<Bit8u>* dosbox_frontbuffer;
extern bool dosbox_frontbuffer_uploaded;
extern Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
extern unsigned RDOSGFXcolorMode;
#ifdef WITH_PINHACK
extern bool request_VGA_SetupDrawing;
#endif
extern bool dosbox_exit;
extern bool frontend_exit;
extern retro_midi_interface retro_midi_interface;
extern bool use_retro_midi;
extern bool have_retro_midi;
extern bool disney_init;
extern std::filesystem::path retro_save_directory;
extern std::filesystem::path retro_system_directory;
extern bool autofire;
extern int mouse_emu_deadzone;
extern float mouse_speed_factor_x;
extern float mouse_speed_factor_y;
extern std::array<bool, 16> connected;
extern bool gamepad[16];
extern bool libretro_supports_bitmasks;
extern int16_t joypad_bits[RETRO_DEVICES];
extern std::set<std::string> locked_dosbox_variables;

namespace retro {
class CoreOptions;
extern CoreOptions core_options;
} // namespace retro

constexpr std::tuple<retro_key, KBD_KEYS> retro_dosbox_map[]{
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

void core_autoexec();
auto update_dosbox_variable(
    bool autoexec, const std::string& section_string, const std::string& var_string,
    const std::string& val_string) -> bool;
