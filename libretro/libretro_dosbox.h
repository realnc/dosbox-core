#pragma once
#include "config.h"
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
extern bool emulated_mouse;
extern std::set<std::string> locked_dosbox_variables;

namespace retro {
class CoreOptions;
extern CoreOptions core_options;
} // namespace retro

void core_autoexec();
auto update_dosbox_variable(
    bool autoexec, const std::string& section_string, const std::string& var_string,
    const std::string& val_string) -> bool;
