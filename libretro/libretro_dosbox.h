#pragma once
#include "config.h"
#include "libretro.h"
#include <cstdint>
#include <filesystem>
#include <string>

#define RETROLOG(msg) printf("%s\n", msg)

extern retro_input_poll_t poll_cb;
extern retro_input_state_t input_cb;
extern retro_environment_t environ_cb;
extern retro_log_printf_t log_cb;
extern bool run_synced;
extern float dosbox_aspect_ratio;
extern Bit8u dosbox_framebuffers[2][1024 * 768 * 4];
extern Bit8u* dosbox_frontbuffer;
extern bool dosbox_frontbuffer_uploaded;
extern Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
extern unsigned RDOSGFXcolorMode;
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
extern bool connected[16];
extern bool gamepad[16];
extern bool emulated_mouse;

namespace retro {
class CoreOptions;
extern CoreOptions core_options;
} // namespace retro

void core_autoexec();
auto update_dosbox_variable(
    bool autoexec, const std::string& section_string, const std::string& var_string,
    const std::string& val_string) -> bool;
