#pragma once
#include "config.h"
#include "libretro.h"
#include <cstdint>
#include <string>

# define RETROLOG(msg) printf("%s\n", msg)

enum core_timing_mode {
    CORE_TIMING_UNSYNCED,
    CORE_TIMING_MATCH_FPS,
    CORE_TIMING_SYNCED
};

extern retro_video_refresh_t video_cb;
extern retro_audio_sample_batch_t audio_batch_cb;
extern retro_input_poll_t poll_cb;
extern retro_input_state_t input_cb;
extern retro_environment_t environ_cb;
extern core_timing_mode core_timing;
extern float dosbox_aspect_ratio;
extern Bit8u dosbox_framebuffers[2][1024 * 768 * 4];
extern Bit8u *dosbox_frontbuffer;
extern bool dosbox_frontbuffer_uploaded;
extern bool dosbox_exit;
extern bool frontend_exit;
extern retro_midi_interface retro_midi_interface;
extern bool use_retro_midi;
extern bool have_retro_midi;

namespace retro {
class CoreOptions;
extern CoreOptions core_options;
}

bool update_dosbox_variable(bool autoexec, std::string section_string, std::string var_string, std::string val_string);
