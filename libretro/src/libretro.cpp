// This is copyrighted software. More information is at the end of this file.
#include "libretro.h"
#include "CoreOptions.h"
#include "control.h"
#include "disk_control.h"
#include "dos/cdrom.h"
#include "dos/drives.h"
#include "dosbox.h"
#include "emu_thread.h"
#include "fake_timing.h"
#include "ints/int10.h"
#include "joystick.h"
#include "libretro-vkbd.h"
#include "libretro_core_options.h"
#include "libretro_dosbox.h"
#include "libretro_message.h"
#include "log.h"
#include "mapper.h"
#include "midi_alsa.h"
#include "midi_win32.h"
#include "mixer.h"
#include "pic.h"
#include "pinhack.h"
#include "programs.h"
#include "render.h"
#include "setup.h"
#include "util.h"
#ifdef ANDROID
    #include "nonlibc.h"
#endif
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <thread>
#ifdef _WIN32
    #include <direct.h>
#else
    #include <unistd.h>
#endif

#define RETRO_DEVICE_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1)

#define CORE_VERSION SVN_VERSION " " GIT_VERSION

#ifdef WITH_FAKE_SDL
bool startup_state_capslock;
bool startup_state_numlock;
#endif

bool autofire;
static bool dosbox_initialiazed = false;

std::set<std::string> disabled_dosbox_variables;
std::set<std::string> disabled_core_options;

Bit32u MIXER_RETRO_GetFrequency();
auto MIXER_RETRO_GetAvailableFrames() noexcept -> Bitu;
void MIXER_CallBack(void* userdata, uint8_t* stream, int len);

extern Bit8u herc_pal;
MachineType machine = MCH_VGA;
SVGACards svgaCard = SVGA_None;

/* input variables */
std::array<bool, RETRO_INPUT_PORTS_MAX> gamepad{}; // True means gamepad, false means joystick.
std::array<bool, RETRO_INPUT_PORTS_MAX> connected;
static bool force_2axis_joystick = false;
int mouse_emu_deadzone = 0;
float mouse_speed_factor_x = 1.0;
float mouse_speed_factor_y = 1.0;
float mouse_speed_hack_factor = 1.0;

/* core option variables */
bool run_synced = true;
static bool use_frame_duping = true;
static bool use_spinlock = false;

/* directories */
std::filesystem::path retro_save_directory;
std::filesystem::path retro_system_directory;
std::filesystem::path load_game_directory;
static std::filesystem::path retro_content_directory;
static const std::string retro_library_name = "DOSBox-core";

/* libretro variables */
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t poll_cb;
retro_input_state_t input_cb;
retro_environment_t environ_cb;
retro_perf_callback perf_cb;

/* DOSBox state */
static std::filesystem::path game_path;
static std::filesystem::path config_path;
bool dosbox_exit;
bool frontend_exit;

/* video variables */
static unsigned currentWidth, currentHeight;
static constexpr float default_fps = 60.0f;
static float currentFPS = default_fps;
static float current_aspect_ratio = 0;

/* audio variables */
static std::vector<int16_t> retro_audio_buffer;
static uint32_t retro_audio_buffer_frames = 0;
struct retro_midi_interface retro_midi_interface;
bool use_retro_midi = false;
bool have_retro_midi = false;
#ifdef HAVE_ALSA
static auto alsa_midi_ports = getAlsaMidiPorts();
#endif
bool disney_init;

/* callbacks */
void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}
void retro_set_audio_sample(retro_audio_sample_t)
{ }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}
void retro_set_input_poll(retro_input_poll_t cb)
{
    poll_cb = cb;
}
void retro_set_input_state(retro_input_state_t cb)
{
    input_cb = cb;
}

// Pending overlay mount.
static bool mount_overlay = true;

// Thread we run dosbox in.
static std::thread emu_thread;

/* helper functions */
static char last_written_character = 0;

auto retro_ticks() -> long
{
    return perf_cb.get_time_usec ? perf_cb.get_time_usec() : 0;
}

static void write_out_buffer(const char* const format, ...)
{
    char buf[2048];
    va_list msg;

    va_start(msg, format);
#ifdef ANDROID
    portable_vsnprintf(buf, 2047, format, msg);
#else
    vsnprintf(buf, 2047, format, msg);
#endif
    va_end(msg);

    Bit16u size = (Bit16u)strlen(buf);
    dos.internal_output = true;
    for (Bit16u i = 0; i < size; i++) {
        Bit8u out;
        Bit16u s = 1;
        if (buf[i] == 0xA && last_written_character != 0xD) {
            out = 0xD;
            DOS_WriteFile(STDOUT, &out, &s);
        }
        last_written_character = out = buf[i];
        DOS_WriteFile(STDOUT, &out, &s);
    }
    dos.internal_output = false;
}

static void write_out(const char* const format, ...)
{
    write_out_buffer("\n");
    write_out_buffer(format);
}

static void mount_overlay_filesystem(const char drive, std::filesystem::path path)
{
    // Make sure the path ends with a dir separator, otherwise dosbox will glitch out when writing
    // to the overlay.
    path.make_preferred();
    auto path_str = path.u8string();
    if (path_str.back() != std::filesystem::path::preferred_separator) {
        path_str += std::filesystem::path::preferred_separator;
    }

    retro::logDebug("Mounting {} in {} as overlay.", path_str, drive);

    if (!Drives[drive - 'A']) {
        retro::logError("Base drive {} is not mounted.", drive);
        write_out("No basedrive mounted yet!");
        return;
    }

    auto* base_drive = dynamic_cast<localDrive*>(Drives[drive - 'A']);
    if (!base_drive || dynamic_cast<cdromDrive*>(base_drive)
        || dynamic_cast<Overlay_Drive*>(base_drive))
    {
        retro::logError("Base drive {} is not compatible.", drive);
        return;
    }

    retro::logDebug("Creating save directory {}.", path_str);
    try {
        std::filesystem::create_directories(path);
    }
    catch (const std::exception& e) {
        retro::logError("Error creating overlay directory {}: {}.", path_str, e.what());
        return;
    }

    // Give the overlay the same size as the base drive.
    Bit16u bytes_per_sector;
    Bit8u sectors_per_cluster;
    Bit16u total_clusters;
    Bit16u free_clusters;
    base_drive->AllocationInfo(
        &bytes_per_sector, &sectors_per_cluster, &total_clusters, &free_clusters);
    Bit8u o_error = 0;
    auto overlay = std::make_unique<Overlay_Drive>(
        base_drive->getBasedir(), path_str.c_str(), bytes_per_sector, sectors_per_cluster,
        total_clusters, free_clusters, 0xF8, o_error);
    if (o_error != 0) {
        if (o_error == 1) {
            retro::logError("Can't mix absolute and relative paths.");
        } else if (o_error == 2) {
            retro::logError("Overlay can't be in the same underlying file system.");
        } else {
            retro::logError("Aomething went wrong while mounting overlay. error code: {}", o_error);
        }
        return;
    }

    mem_writeb(Real2Phys(dos.tables.mediaid) + (drive - 'A') * 9, overlay->GetMediaByte());
    overlay->dirCache.SetLabel((drive + std::string("_OVERLAY")).c_str(), false, false);
    // Preserve current working directory if not marked as deleted.
    if (overlay->TestDir(base_drive->curdir)) {
        std::strncpy(overlay->curdir, base_drive->curdir, DOS_PATHLENGTH);
        overlay->curdir[DOS_PATHLENGTH - 1] = '\0';
    }
    Drives[drive - 'A'] = overlay.release();
    delete base_drive;
}

static auto compare_dosbox_variable(
    const std::string& section_string, const std::string& var_string, const std::string& val_string)
    -> bool
{
    bool ret = false;
    Section* section = control->GetSection(section_string);
    Section_prop* secprop = static_cast<Section_prop*>(section);
    if (secprop) {
        ret = section->GetPropValue(var_string) == val_string;
    }
    return ret;
}

auto update_dosbox_variable(
    const bool autoexec, const std::string& section_string, const std::string& var_string,
    const std::string& val_string) -> bool
{
    bool ret = false;
    if (dosbox_initialiazed && compare_dosbox_variable(section_string, var_string, val_string)) {
        return false;
    }

    if (disabled_dosbox_variables.count(var_string) != 0
        && retro::core_options[CORE_OPT_OPTION_HANDLING].toString() == "disable")
    {
        return false;
    }

    disable_core_opt_sync = true;
    Section* section = control->GetSection(section_string);
    Section_prop* secprop = static_cast<Section_prop*>(section);
    if (secprop) {
        if (!autoexec) {
            section->ExecuteDestroy(false);
        }
        std::string inputline = var_string + "=" + val_string;
        ret = section->HandleInputline(inputline.c_str());
        if (!autoexec) {
            section->ExecuteInit(false);
        }
    }
    disable_core_opt_sync = false;
    retro::logDebug("Variable {}::{} updated to {}.", section_string, var_string, val_string);
    return ret;
}

static auto queue_audio() -> Bitu
{
    const auto available_audio_frames = MIXER_RETRO_GetAvailableFrames();

    if (available_audio_frames > 0) {
        const auto samples = available_audio_frames * 2;
        const auto bytes = available_audio_frames * 4;

        if (samples > retro_audio_buffer.capacity()) {
            retro_audio_buffer.reserve(samples);
            retro::logDebug("Output audio buffer resized to {} samples.\n", samples);
        }
        MIXER_CallBack(nullptr, reinterpret_cast<Uint8*>(retro_audio_buffer.data()), bytes);
    }
    return available_audio_frames;
}

static void leave_thread(const Bitu /*val*/)
{
    retro_audio_buffer_frames = queue_audio();
    switchThread();

    if (!run_synced) {
        /* Schedule the next frontend interrupt */
        PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
    }
}

void update_mouse_speed_fix()
{
    if (!retro::core_options[CORE_OPT_MOUSE_SPEED_HACK].toBool()) {
        mouse_speed_hack_factor = 1.0f;
        return;
    }

    switch (currentHeight) {
    case 240:
    case 480:
    case 720:
    case 960:
    case 1200:
        mouse_speed_hack_factor = 2.0f;
        break;
    default:
        mouse_speed_hack_factor = 0.85f;
    }
}

static void update_gfx_mode(const bool change_fps)
{
    const float old_fps = currentFPS;
    retro_system_av_info new_av_info;
    bool cb_error = false;

    retro_get_system_av_info(&new_av_info);

    if (change_fps) {
        const float new_fps = run_synced ? render.src.fps : default_fps;
        new_av_info.timing.fps = new_fps;
        new_av_info.timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
        cb_error = !environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &new_av_info);
        if (cb_error) {
            retro::logError("SET_SYSTEM_AV_INFO failed.");
        }
        currentFPS = new_fps;
    } else {
        cb_error = !environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &new_av_info);
        if (cb_error) {
            retro::logError("SET_GEOMETRY failed.");
        }
    }

    if (!cb_error) {
        retro::logInfo(
            "Resolution changed {}x{} @ {:.9}Hz AR: {:.5} => {}x{} @ {:.9}Hz AR: {:.5}.",
            currentWidth, currentHeight, old_fps, current_aspect_ratio, RDOSGFXwidth, RDOSGFXheight,
            currentFPS, dosbox_aspect_ratio);
    }

    currentWidth = RDOSGFXwidth;
    currentHeight = RDOSGFXheight;
    current_aspect_ratio = dosbox_aspect_ratio;

    update_mouse_speed_fix();
}

static RETRO_CALLCONV auto update_core_option_visibility() -> bool
{
    using namespace retro;

    bool updated = false;
    const bool show_all = core_options[CORE_OPT_ADV_OPTIONS].toBool();

    const auto& mode = core_options[CORE_OPT_CPU_CYCLES_MODE].toString();
    updated |= core_options.setVisible(CORE_OPT_CPU_CYCLES_LIMIT, mode == "max" || mode == "auto");
    updated |= core_options.setVisible(
        {CORE_OPT_CPU_CYCLES_MULTIPLIER_REALMODE, CORE_OPT_CPU_CYCLES_REALMODE,
         CORE_OPT_CPU_CYCLES_MULTIPLIER_FINE_REALMODE, CORE_OPT_CPU_CYCLES_FINE_REALMODE},
        mode == "auto");

    const bool mpu_enabled = core_options[CORE_OPT_MPU_TYPE].toString() != "none";
    updated |= core_options.setVisible(CORE_OPT_MIDI_DRIVER, mpu_enabled);

    const auto& midi_driver = core_options[CORE_OPT_MIDI_DRIVER].toString();
#ifdef WITH_BASSMIDI
    const auto bassmidi_enabled = mpu_enabled && midi_driver == "bassmidi";
    updated |= core_options.setVisible(
        {CORE_OPT_BASSMIDI_SOUNDFONT, CORE_OPT_BASSMIDI_SFVOLUME}, bassmidi_enabled);
    updated |= core_options.setVisible(CORE_OPT_BASSMIDI_VOICES, show_all && bassmidi_enabled);
#endif

#ifdef WITH_FLUIDSYNTH
    const auto fsynth_enabled = mpu_enabled && midi_driver == "fluidsynth";
    for (const auto* name :
         {CORE_OPT_FLUID_SOUNDFONT, CORE_OPT_FLUID_GAIN, CORE_OPT_FLUID_POLYPHONY,
          CORE_OPT_FLUID_CORES})
    {
        updated |= core_options.setVisible(name, fsynth_enabled);
    }
    for (const auto* name :
         {CORE_OPT_FLUID_SAMPLERATE, CORE_OPT_FLUID_REVERB, CORE_OPT_FLUID_REVERB_ROOMSIZE,
          CORE_OPT_FLUID_REVERB_DAMPING, CORE_OPT_FLUID_REVERB_WIDTH, CORE_OPT_FLUID_REVERB_LEVEL,
          CORE_OPT_FLUID_CHORUS, CORE_OPT_FLUID_CHORUS_NUMBER, CORE_OPT_FLUID_CHORUS_LEVEL,
          CORE_OPT_FLUID_CHORUS_SPEED, CORE_OPT_FLUID_CHORUS_DEPTH})
    {
        updated |= core_options.setVisible(name, show_all && fsynth_enabled);
    }
#endif

    const auto mt32_enabled = mpu_enabled && midi_driver == "mt32";
    for (const auto* name :
         {CORE_OPT_MT32_TYPE, CORE_OPT_MT32_THREAD, CORE_OPT_MT32_PARTIALS, CORE_OPT_MT32_ANALOG})
    {
        updated |= core_options.setVisible(name, mt32_enabled);
    }
    for (const auto* name :
         {CORE_OPT_MT32_REVERSE_STEREO, CORE_OPT_MT32_DAC, CORE_OPT_MT32_REVERB_MODE,
          CORE_OPT_MT32_REVERB_TIME, CORE_OPT_MT32_REVERB_LEVEL, CORE_OPT_MT32_RATE,
          CORE_OPT_MT32_SRC_QUALITY, CORE_OPT_MT32_NICEAMPRAMP})
    {
        updated |= core_options.setVisible(name, show_all && mt32_enabled);
    }
    const bool mt32_is_threaded = core_options[CORE_OPT_MT32_THREAD].toBool();
    for (const auto* name : {CORE_OPT_MT32_CHUNK, CORE_OPT_MT32_PREBUFFER}) {
        updated |= core_options.setVisible(name, show_all && mt32_enabled && mt32_is_threaded);
    }

#ifdef HAVE_ALSA
    updated |= core_options.setVisible(CORE_OPT_MIDI_PORT, midi_driver == "alsa" && mpu_enabled);
#endif
#ifdef __WIN32__
    updated |= core_options.setVisible(CORE_OPT_MIDI_PORT, midi_driver == "win32" && mpu_enabled);
#endif

#ifdef WITH_VOODOO
    updated |= core_options.setVisible(
        CORE_OPT_VOODOO_MEMORY_SIZE, core_options[CORE_OPT_VOODOO].toString() != "false");
#endif

    const auto& machine_type = core_options[CORE_OPT_MACHINE_TYPE].toString();

    updated |= core_options.setVisible(
        {CORE_OPT_MACHINE_CGA_COMPOSITE_MODE, CORE_OPT_MACHINE_CGA_MODEL},
        show_all && machine_type == "cga");

    updated |= core_options.setVisible(
        CORE_OPT_MACHINE_HERCULES_PALETTE, show_all && machine_type == "hercules");

    const auto& sb_type = core_options[CORE_OPT_SBLASTER_TYPE].toString();
    const bool sb_enabled = sb_type != "none";
    const bool sb_is_gameblaster = sb_type == "gb";
    const bool sb_is_sb16 = sb_type == "sb16";
    updated |= core_options.setVisible(
        {CORE_OPT_SBLASTER_BASE, CORE_OPT_SBMIXER, CORE_OPT_SBLASTER_OPL_MODE,
         CORE_OPT_SBLASTER_OPL_EMU},
        show_all && sb_enabled);
    updated |= core_options.setVisible(
        {CORE_OPT_SBLASTER_IRQ, CORE_OPT_SBLASTER_DMA},
        show_all && sb_enabled && !sb_is_gameblaster);
    updated |= core_options.setVisible(CORE_OPT_SBLASTER_HDMA, show_all && sb_is_sb16);

    auto gus_enabled = core_options[CORE_OPT_GUS].toBool();
    updated |= core_options.setVisible(
        {CORE_OPT_GUSBASE, CORE_OPT_GUSIRQ, CORE_OPT_GUSDMA}, show_all && gus_enabled);

    updated |= core_options.setVisible(
        {CORE_OPT_DEFAULT_MOUNT_FREESIZE, CORE_OPT_THREAD_SYNC, CORE_OPT_CPU_TYPE, CORE_OPT_SCALER,
         CORE_OPT_MPU_TYPE, CORE_OPT_TANDY, CORE_OPT_DISNEY, CORE_OPT_LOG_METHOD,
         CORE_OPT_LOG_LEVEL},
        show_all);

#ifdef WITH_PINHACK
    const auto pinhack_enabled = core_options[CORE_OPT_PINHACK].toBool();
    const bool pinhack_expand_height_enabled =
        core_options[CORE_OPT_PINHACKEXPANDHEIGHT_COARSE].toInt() != 0;
    updated |= core_options.setVisible(
        {CORE_OPT_PINHACKACTIVE, CORE_OPT_PINHACKTRIGGERWIDTH, CORE_OPT_PINHACKTRIGGERHEIGHT,
         CORE_OPT_PINHACKEXPANDHEIGHT_COARSE},
        pinhack_enabled);
    updated |= core_options.setVisible(
        CORE_OPT_PINHACKEXPANDHEIGHT_FINE, pinhack_enabled && pinhack_expand_height_enabled);
#endif

    static const std::regex pad_map_regex{"^pad(0|1)_map_"};
    updated |=
        core_options.setVisible(pad_map_regex, core_options[CORE_OPT_SHOW_KB_MAP_OPTIONS].toBool());

    for (const auto& option_name : disabled_core_options) {
        if (core_options.option(option_name)) {
            updated |= core_options.setVisible(option_name, false);
        }
    }
    return updated;
}

static void check_blaster_variables(const bool autoexec)
{
    using namespace retro;

    update_dosbox_variable(
        autoexec, "sblaster", "sbtype", core_options[CORE_OPT_SBLASTER_TYPE].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "sbbase", core_options[CORE_OPT_SBLASTER_BASE].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "irq", core_options[CORE_OPT_SBLASTER_IRQ].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "dma", core_options[CORE_OPT_SBLASTER_DMA].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "hdma", core_options[CORE_OPT_SBLASTER_HDMA].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "sbmixer", core_options[CORE_OPT_SBMIXER].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplmode", core_options[CORE_OPT_SBLASTER_OPL_MODE].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplemu", core_options[CORE_OPT_SBLASTER_OPL_EMU].toString());
}

static void check_gus_variables(const bool autoexec)
{
    using namespace retro;

    update_dosbox_variable(autoexec, "gus", "gus", core_options[CORE_OPT_GUS].toString());
    update_dosbox_variable(autoexec, "gus", "gusbase", core_options[CORE_OPT_GUSBASE].toString());
    update_dosbox_variable(autoexec, "gus", "gusirq", core_options[CORE_OPT_GUSIRQ].toString());
    update_dosbox_variable(autoexec, "gus", "gusdma", core_options[CORE_OPT_GUSDMA].toString());
}

static void check_vkbd_variables()
{
    using namespace retro;

    const auto& theme = core_options[CORE_OPT_VKBD_THEME].toString();
    if (theme.find("light") != std::string::npos) {
        opt_vkbd_theme = 1;
    } else if (theme.find("dark") != std::string::npos) {
        opt_vkbd_theme = 2;
    }
    if (theme.find("outline") != std::string::npos) {
        opt_vkbd_theme |= 0x80;
    }

    const auto& transparency = core_options[CORE_OPT_VKBD_TRANSPARENCY].toString();
    if (transparency == "0%") {
        opt_vkbd_alpha = GRAPH_ALPHA_100;
    } else if (transparency == "25%") {
        opt_vkbd_alpha = GRAPH_ALPHA_75;
    } else if (transparency == "50%") {
        opt_vkbd_alpha = GRAPH_ALPHA_50;
    } else if (transparency == "75%") {
        opt_vkbd_alpha = GRAPH_ALPHA_25;
    } else if (transparency == "100%") {
        opt_vkbd_alpha = GRAPH_ALPHA_0;
    }
}

static void check_pinhack_variables()
{
#ifdef WITH_PINHACK
    using namespace retro;
    bool updated = false;

    for (const auto* name :
         {CORE_OPT_PINHACK, CORE_OPT_PINHACKACTIVE, CORE_OPT_PINHACKTRIGGERWIDTH,
          CORE_OPT_PINHACKTRIGGERHEIGHT})
    {
        updated |= update_dosbox_variable(false, "pinhack", name, core_options[name].toString());
    }

    const int expand_height_coarse = core_options[CORE_OPT_PINHACKEXPANDHEIGHT_COARSE].toInt();
    if (expand_height_coarse > 0) {
        const int expand_height_fine = core_options[CORE_OPT_PINHACKEXPANDHEIGHT_FINE].toInt();
        updated |= update_dosbox_variable(
            false, "pinhack", "pinhackexpandheight",
            std::to_string(expand_height_coarse + expand_height_fine));
    }

    if (updated) {
        request_VGA_SetupDrawing = true;
    }
#endif
}

void core_autoexec()
{
    check_blaster_variables(true);
    check_gus_variables(true);
}

static auto make_cpu_cycles_string() -> std::string
{
    using namespace retro;

    const auto& mode = core_options[CORE_OPT_CPU_CYCLES_MODE].toString();
    const int realmode_cycles = core_options[CORE_OPT_CPU_CYCLES_REALMODE].toInt()
            * core_options[CORE_OPT_CPU_CYCLES_MULTIPLIER_REALMODE].toInt()
        + core_options[CORE_OPT_CPU_CYCLES_FINE_REALMODE].toInt()
            * core_options[CORE_OPT_CPU_CYCLES_MULTIPLIER_FINE_REALMODE].toInt();
    const int cycles = core_options[CORE_OPT_CPU_CYCLES].toInt()
            * core_options[CORE_OPT_CPU_CYCLES_MULTIPLIER].toInt()
        + core_options[CORE_OPT_CPU_CYCLES_FINE].toInt()
            * core_options[CORE_OPT_CPU_CYCLES_MULTIPLIER_FINE].toInt();
    const auto cycles_str = std::to_string(cycles);
    const std::string max_limits_str = [cycles, &cycles_str] {
        const auto& limit = core_options[CORE_OPT_CPU_CYCLES_LIMIT].toString();
        std::string retval;
        if (limit != "none") {
            retval += ' ' + limit;
        }
        if (cycles > 0) {
            retval += " limit " + cycles_str;
        }
        return retval;
    }();

    std::string cycles_string = mode;
    if (mode == "fixed" && cycles > 0) {
        cycles_string += ' ' + cycles_str;
    } else if (mode == "max") {
        cycles_string += max_limits_str;
    } else {
        if (realmode_cycles > 0) {
            cycles_string += ' ' + std::to_string(realmode_cycles);
        }
        cycles_string += max_limits_str;
    }
    return cycles_string;
}

static void check_cpu_cycle_variables()
{
    update_dosbox_variable(
        false, "cpu", "cycleup", retro::core_options[CORE_OPT_CPU_CYCLES_UP].toString());
    update_dosbox_variable(
        false, "cpu", "cycledown", retro::core_options[CORE_OPT_CPU_CYCLES_DOWN].toString());

    if (disabled_dosbox_variables.count("cycles") != 0) {
        return;
    }

    static std::string prev_cycles_string;
    auto new_cycles_string = make_cpu_cycles_string();

    if (prev_cycles_string != new_cycles_string) {
        update_dosbox_variable(false, "cpu", "cycles", new_cycles_string);
        prev_cycles_string = std::move(new_cycles_string);
    }
}

static void update_bassmidi_variables()
{
#ifdef WITH_BASSMIDI
    const auto soundfont = retro_system_directory / "soundfonts"
        / retro::core_options[CORE_OPT_BASSMIDI_SOUNDFONT].toString();
    update_dosbox_variable(false, "bassmidi", "bassmidi.soundfont", soundfont.u8string());

    for (const auto* name : {CORE_OPT_BASSMIDI_SFVOLUME, CORE_OPT_BASSMIDI_VOICES}) {
        update_dosbox_variable(false, "bassmidi", name, retro::core_options[name].toString());
    }
#endif
}

static void update_fsynth_variables()
{
#ifdef WITH_FLUIDSYNTH
    const auto soundfont = retro_system_directory / "soundfonts"
        / retro::core_options[CORE_OPT_FLUID_SOUNDFONT].toString();
    update_dosbox_variable(false, "midi", "fluid.soundfont", soundfont.u8string());

    for (const auto* name :
         {CORE_OPT_FLUID_GAIN, CORE_OPT_FLUID_POLYPHONY, CORE_OPT_FLUID_CORES,
          CORE_OPT_FLUID_SAMPLERATE, CORE_OPT_FLUID_REVERB, CORE_OPT_FLUID_REVERB_ROOMSIZE,
          CORE_OPT_FLUID_REVERB_DAMPING, CORE_OPT_FLUID_REVERB_WIDTH, CORE_OPT_FLUID_REVERB_LEVEL,
          CORE_OPT_FLUID_CHORUS, CORE_OPT_FLUID_CHORUS_NUMBER, CORE_OPT_FLUID_CHORUS_LEVEL,
          CORE_OPT_FLUID_CHORUS_SPEED, CORE_OPT_FLUID_CHORUS_DEPTH})
    {
        update_dosbox_variable(false, "midi", name, retro::core_options[name].toString());
    }
#endif
}

static void update_mt32_variables()
{
    update_dosbox_variable(false, "midi", "mt32.romdir", retro_system_directory.u8string());

    for (const auto* name :
         {CORE_OPT_MT32_TYPE, CORE_OPT_MT32_THREAD, CORE_OPT_MT32_PARTIALS, CORE_OPT_MT32_ANALOG,
          CORE_OPT_MT32_REVERSE_STEREO, CORE_OPT_MT32_DAC, CORE_OPT_MT32_REVERB_MODE,
          CORE_OPT_MT32_REVERB_TIME, CORE_OPT_MT32_REVERB_LEVEL, CORE_OPT_MT32_RATE,
          CORE_OPT_MT32_SRC_QUALITY, CORE_OPT_MT32_NICEAMPRAMP, CORE_OPT_MT32_CHUNK,
          CORE_OPT_MT32_PREBUFFER})
    {
        update_dosbox_variable(false, "midi", name, retro::core_options[name].toString());
    }
}

static void use_libretro_log_cb()
{
    retro_log_callback log_cb{nullptr};
    if (!environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_cb)) {
        retro::logError("RETRO_ENVIRONMENT_GET_LOG_INTERFACE failed.");
        log_cb.log = nullptr; // paranoia
    } else if (log_cb.log) {
        retro::setRetroLogCb(log_cb.log);
    }
}

static void update_libretro_log_interface()
{
    retro_log_callback log_cb{nullptr};
    if (retro::core_options[CORE_OPT_LOG_METHOD].toString() == "frontend") {
        use_libretro_log_cb();
    } else {
        retro::setRetroLogCb(log_cb.log);
    }
}

static void update_log_verbosity()
{
    const auto& level = retro::core_options[CORE_OPT_LOG_LEVEL].toString();
    if (level == "errors") {
        retro::setLoggingLevel(RETRO_LOG_ERROR);
    } else if (level == "warnings") {
        retro::setLoggingLevel(RETRO_LOG_WARN);
    } else if (level == "info") {
        retro::setLoggingLevel(RETRO_LOG_INFO);
    } else {
        retro::setLoggingLevel(RETRO_LOG_DEBUG);
    }
}

static void check_variables()
{
    using namespace retro;

    update_log_verbosity();
    update_libretro_log_interface();

    {
        const bool old_timing = run_synced;
        run_synced = core_options[CORE_OPT_CORE_TIMING].toString() == "external";

        if (dosbox_initialiazed && run_synced != old_timing) {
            if (!run_synced) {
                PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
            }
            update_gfx_mode(true);
        }
    }

    use_frame_duping = core_options[CORE_OPT_FRAME_DUPING].toBool();
    use_spinlock = core_options[CORE_OPT_THREAD_SYNC].toString() == "spin";
    useSpinlockThreadSync(use_spinlock);

    if (!dosbox_initialiazed) {
        update_dosbox_variable(
            false, "dosbox", "memsize", core_options[CORE_OPT_MEMORY_SIZE].toString());

        svgaCard = SVGA_None;
        machine = MCH_VGA;
        int10.vesa_nolfb = false;
        int10.vesa_oldvbe = false;
        const std::string& machine_type = core_options[CORE_OPT_MACHINE_TYPE].toString();
        if (machine_type == "hercules") {
            machine = MCH_HERC;
        } else if (machine_type == "cga") {
            machine = MCH_CGA;
        } else if (machine_type == "pcjr") {
            machine = MCH_PCJR;
        } else if (machine_type == "tandy") {
            machine = MCH_TANDY;
        } else if (machine_type == "ega") {
            machine = MCH_EGA;
        } else if (machine_type == "svga_s3") {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
        } else if (machine_type == "svga_et4000") {
            machine = MCH_VGA;
            svgaCard = SVGA_TsengET4K;
        } else if (machine_type == "svga_et3000") {
            machine = MCH_VGA;
            svgaCard = SVGA_TsengET3K;
        } else if (machine_type == "svga_paradise") {
            machine = MCH_VGA;
            svgaCard = SVGA_ParadisePVGA1A;
        } else if (machine_type == "vesa_nolfb") {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
            int10.vesa_nolfb = true;
        } else if (machine_type == "vesa_nolfb") {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
            int10.vesa_oldvbe = true;
        } else {
            machine = MCH_VGA;
            svgaCard = SVGA_None;
        }
        update_dosbox_variable(false, "dosbox", "machine", machine_type);

#ifdef WITH_VOODOO
        update_dosbox_variable(false, "pci", "voodoo", core_options[CORE_OPT_VOODOO].toString());
        update_dosbox_variable(
            false, "pci", "voodoomem", core_options[CORE_OPT_VOODOO_MEMORY_SIZE].toString());
#endif

        mount_overlay = core_options[CORE_OPT_SAVE_OVERLAY].toBool();
    } else {
        update_dosbox_variable(false, "dos", "xms", core_options[CORE_OPT_XMS].toString());
        update_dosbox_variable(false, "dos", "ems", core_options[CORE_OPT_EMS].toString());
        update_dosbox_variable(false, "dos", "umb", core_options[CORE_OPT_UMB].toString());

        if (machine == MCH_HERC) {
            herc_pal = core_options[CORE_OPT_MACHINE_HERCULES_PALETTE].toInt();
            Herc_Palette();
            VGA_DAC_CombineColor(1, 7);
        } else if (machine == MCH_CGA) {
            CGA_Composite_Mode(core_options[CORE_OPT_MACHINE_CGA_COMPOSITE_MODE].toInt());
            CGA_Model(core_options[CORE_OPT_MACHINE_CGA_MODEL].toInt());
        }

        check_blaster_variables(false);
        check_gus_variables(false);

        {
            const bool prev_force_2axis_joystick = force_2axis_joystick;
            const int prev_mouse_emu_deadzone = mouse_emu_deadzone;

            force_2axis_joystick = core_options[CORE_OPT_JOYSTICK_FORCE_2AXIS].toBool();
            mouse_emu_deadzone = core_options[CORE_OPT_EMULATED_MOUSE_DEADZONE].toInt();
            if (prev_force_2axis_joystick != force_2axis_joystick
                || prev_mouse_emu_deadzone != mouse_emu_deadzone)
            {
                MAPPER_Init();
            }
        }

        for (const auto& option :
             {CORE_OPT_MOUSE_SPEED_X, CORE_OPT_MOUSE_SPEED_Y, CORE_OPT_MOUSE_SPEED_MULT,
              CORE_OPT_MOUSE_SPEED_HACK})
        {
            update_dosbox_variable(false, "dosbox-core", option, core_options[option].toString());
        }

        update_dosbox_variable(false, "cpu", "cputype", core_options[CORE_OPT_CPU_TYPE].toString());
        update_dosbox_variable(false, "cpu", "core", core_options[CORE_OPT_CPU_CORE].toString());
        update_dosbox_variable(
            false, "render", "aspect", core_options[CORE_OPT_ASPECT_CORRECTION].toString());
        update_dosbox_variable(false, "render", "scaler", core_options[CORE_OPT_SCALER].toString());
        update_dosbox_variable(
            false, "joystick", "timed", core_options[CORE_OPT_JOYSTICK_TIMED].toString());

        check_cpu_cycle_variables();

        update_dosbox_variable(
            false, "speaker", "pcspeaker", core_options[CORE_OPT_PCSPEAKER].toString());

        {
            const auto& mpu_type = core_options[CORE_OPT_MPU_TYPE].toString();
            update_dosbox_variable(false, "midi", "mpu401", mpu_type);

            const auto& midi_driver = core_options[CORE_OPT_MIDI_DRIVER].toString();
            use_retro_midi = midi_driver == "libretro";
            update_dosbox_variable(
                false, "midi", "mididevice", use_retro_midi ? "none" : midi_driver);

            update_bassmidi_variables();
            update_fsynth_variables();
            update_mt32_variables();

            if (use_retro_midi && !have_retro_midi) {
                have_retro_midi =
                    environ_cb(RETRO_ENVIRONMENT_GET_MIDI_INTERFACE, &retro_midi_interface);
                retro::logDebug(
                    "Libretro MIDI interface {}.", have_retro_midi ? "initialized" : "unavailable");
            }
#if defined(HAVE_ALSA)
            // Dosbox only accepts the numerical MIDI port, not client/port names.
            const auto& current_value = core_options[CORE_OPT_MIDI_PORT].toString();
            std::string first_gm_port;
            bool port_found = false;
            for (const auto& [midi_std, port, client, port_name] : alsa_midi_ports) {
                if ((current_value == "auto gm" && midi_std == MidiStandard::GM)
                    || (current_value == "auto gs gm" && midi_std == MidiStandard::GS)
                    || (current_value == "auto xg" && midi_std == MidiStandard::XG)
                    || (current_value == "auto mt32" && midi_std == MidiStandard::MT32)
                    || (current_value == "auto gm2" && midi_std == MidiStandard::GM2)
                    || (client + ':' + port_name == current_value))
                {
                    update_dosbox_variable(false, "midi", "midiconfig", port);
                    port_found = true;
                    break;
                }
                if (midi_std == MidiStandard::GM && first_gm_port.empty()) {
                    first_gm_port = std::move(port);
                }
            }
            // Fall back to the first GM port if we originally wanted GS but didn't find one.
            if (!port_found && current_value == "auto gs gm" && !first_gm_port.empty()) {
                update_dosbox_variable(false, "midi", "midiconfig", first_gm_port);
            }
#endif
#ifdef __WIN32__
            update_dosbox_variable(
                false, "midi", "midiconfig", core_options[CORE_OPT_MIDI_PORT].toString());
#endif
        }

#if defined(C_IPX)
        update_dosbox_variable(false, "ipx", "ipx", core_options[CORE_OPT_IPX].toString());
#endif

        update_dosbox_variable(false, "speaker", "tandy", core_options[CORE_OPT_TANDY].toString());

        disney_init = core_options[CORE_OPT_DISNEY].toBool();
        update_dosbox_variable(false, "speaker", "disney", disney_init ? "true" : "false");

        check_vkbd_variables();
        check_pinhack_variables();
    }
}

static void start_dosbox(const std::string cmd_line)
{
    const char* const argv[2] = {"dosbox", cmd_line.c_str()};
    CommandLine com_line(cmd_line.empty() ? 1 : 2, argv);
    Config myconf(&com_line);
    control = &myconf;
    dosbox_initialiazed = false;

    /* Init the configuration system and add default values */
    DOSBOX_Init();

    /* Load config */
    if (!config_path.empty()) {
        control->ParseConfigFile(config_path.u8string().c_str());
    }

    check_variables();
    control->Init();

    /* Init done, go back to the main thread */
    switchThread();

    dosbox_initialiazed = true;
    check_variables();

    if (!run_synced) {
        /* When not synced, schedule the first frontend interrupt */
        PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
    }

    try {
        control->StartUp();
    }
    catch (const EmuThreadCanceled&) {
        retro::logDebug("Frontend asked to exit.");
        return;
    }

    retro::logDebug("Core asked to exit.");
    dosbox_exit = true;
    switchThread();
}

void restart_program(std::vector<std::string>& /*parameters*/)
{
    retro::logWarn("Program restart not supported.");
}

auto retro_api_version() -> unsigned
{
    return RETRO_API_VERSION;
}

void retro_set_environment(const retro_environment_t cb)
{
    environ_cb = cb;

    bool allow_no_game = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &allow_no_game);

    static constexpr std::array<retro_controller_description, 4> ports_default{{
        {"Keyboard + Mouse", RETRO_DEVICE_KEYBOARD},
        {"Gamepad", RETRO_DEVICE_JOYPAD},
        {"Joystick", RETRO_DEVICE_JOYSTICK},
        {"Disconnected", RETRO_DEVICE_NONE},
    }};

    static std::array<retro_controller_info, RETRO_INPUT_PORTS_MAX + 1> ports = [] {
        decltype(ports) tmp;
        tmp.fill({ports_default.data(), ports_default.size()});
        tmp.back() = {};
        return tmp;
    }();
    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, ports.data());

    retro_core_options_update_display_callback display_cb{update_core_option_visibility};
    environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_UPDATE_DISPLAY_CALLBACK, &display_cb);
}

void retro_set_controller_port_device(const unsigned port, const unsigned device)
{
    if (static_cast<int>(port) >= RETRO_INPUT_PORTS_MAX) {
        retro::logWarn(
            "Ignoring controller port {} since we only support {} ports.", port + 1,
            RETRO_INPUT_PORTS_MAX);
        return;
    }

    connected[port] = false;
    gamepad[port] = false;
    switch (device) {
    case RETRO_DEVICE_JOYPAD:
        connected[port] = true;
        gamepad[port] = true;
        break;
    case RETRO_DEVICE_JOYSTICK:
        connected[port] = true;
        gamepad[port] = false;
        break;
    case RETRO_DEVICE_KEYBOARD:
    default:
        connected[port] = false;
        gamepad[port] = false;
        break;
    }
    MAPPER_Init();
}

void retro_get_system_info(retro_system_info* const info)
{
    info->library_name = retro_library_name.c_str();
    info->library_version = CORE_VERSION;
    info->valid_extensions = "exe|com|bat|conf|cue|iso|img";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(retro_system_av_info* const info)
{
    info->geometry.base_width = RDOSGFXwidth;
    info->geometry.base_height = RDOSGFXheight;
    info->geometry.max_width = GFX_MAX_WIDTH;
    info->geometry.max_height = GFX_MAX_HEIGHT;
    info->geometry.aspect_ratio = dosbox_aspect_ratio;
    info->timing.fps = currentFPS;
    info->timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
}

void retro_init()
{
    use_libretro_log_cb();
    retro::setMessageEnvCb(environ_cb);

    retro_audio_buffer.reserve(4096);

    RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_XRGB8888;
    retro::logDebug("Setting pixel format to RETRO_PIXEL_FORMAT_XRGB8888.");
    if (!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RDOSGFXcolorMode)) {
        retro::logError("RETRO_ENVIRONMENT_SET_PIXEL_FORMAT failed.");
    }

    if (const char* system_dir = nullptr;
        !environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir))
    {
        retro::logError("RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY failed.");
    } else if (system_dir) {
        retro_system_directory = std::filesystem::path(system_dir).make_preferred();
        retro::logDebug("System directory: {}", retro_system_directory);
    }

    if (const char* save_dir = nullptr;
        !environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir)) {
        retro::logError("RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY failed.");
    } else if (save_dir) {
        retro_save_directory = std::filesystem::path(save_dir).make_preferred();
        retro::logDebug("Save directory: {}", retro_save_directory);
    }

    if (const char* content_dir = nullptr;
        !environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir))
    {
        retro::logError("RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY failed.");
    } else if (content_dir) {
        retro_content_directory = std::filesystem::path(content_dir).make_preferred();
        retro::logDebug("Core assets directory: {}", retro_content_directory);
    }

#ifdef HAVE_ALSA
    // Add detected MIDI ports as additional values to the midi port option.
    {
        auto* const option = retro::core_options.option(CORE_OPT_MIDI_PORT);
        // We don't use numerical ports since these can change. We instead use the MIDI client and
        // port name and resolve them back to numerical ports later on.
        for (const auto& [midi_std, port, client, port_name] : alsa_midi_ports) {
            option->addValue(
                {client + ':' + port_name, "[" + client + "] " + port_name + " - " + port});
        }
    }
#endif
#ifdef __WIN32__
    {
        std::vector<retro::CoreOptionValue> values;
        for (const auto& port : getWin32MidiPorts()) {
            values.emplace_back(port);
        }
        if (values.empty()) {
            values.emplace_back("none", "(no MIDI ports found)");
        }
        retro::core_options.option(CORE_OPT_MIDI_PORT)->setValues(values, values.front());
    }
#endif

    {
        std::vector<retro::CoreOptionValue> fsynth_values;
        std::vector<retro::CoreOptionValue> bass_values;
        try {
            namespace fs = std::filesystem;
            for (const auto& file : fs::directory_iterator(retro_system_directory / "soundfonts")) {
                const auto extension = lower_case(file.path().extension().string());
                if (extension == ".sfz") {
                    bass_values.emplace_back(file.path().filename().u8string());
                    continue;
                }
                if (extension == ".sf2") {
                    bass_values.emplace_back(file.path().filename().u8string());
                    fsynth_values.emplace_back(file.path().filename().u8string());
                    continue;
                }
                for (const char* ext : {".sf3", ".dls", ".gig"}) {
                    if (extension == ext) {
                        fsynth_values.emplace_back(file.path().filename().u8string());
                    }
                }
            }
        }
        catch (const std::exception& e) {
            retro::logError("Error reading soundfont directory: {}", e.what());
        }
        if (bass_values.empty()) {
            bass_values.push_back({"none", "(no soundfonts found)"});
        }
        if (fsynth_values.empty()) {
            fsynth_values.push_back({"none", "(no soundfonts found)"});
        }
#ifdef WITH_BASSMIDI
        retro::core_options.option(CORE_OPT_BASSMIDI_SOUNDFONT)
            ->setValues(bass_values, bass_values.front());
#endif
#ifdef WITH_FLUIDSYNTH
        retro::core_options.option(CORE_OPT_FLUID_SOUNDFONT)
            ->setValues(fsynth_values, fsynth_values.front());
#endif
    }

    {
        bool flag = true;
        environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_ACHIEVEMENTS, &flag);
    }

    retro::core_options.setEnvironmentCallback(environ_cb);
    retro::core_options.updateFrontend();
    update_core_option_visibility();
    disk_control::init(environ_cb);

    if (!environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb)) {
        perf_cb.get_time_usec = nullptr;
    }

    libretro_supports_bitmasks = environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, nullptr);
    retro::logDebug("Frontend input bitmask support: {}", libretro_supports_bitmasks);
}

void retro_deinit()
{
    frontend_exit = true;
    if (emu_thread.joinable()) {
        if (!dosbox_exit) {
            switchThread();
        }
        emu_thread.join();
    }

    libretro_graph_free();
}

auto retro_load_game(const retro_game_info* const game) -> bool
{
    std::filesystem::path load_path;
    std::filesystem::path disk_load_image;

    if (game && game->path) {
        load_path = std::filesystem::path(game->path).make_preferred();
        game_path = load_path;
    }

    if (const auto extension = lower_case(load_path.extension().string()); extension == ".conf") {
        config_path = std::filesystem::absolute(load_path);
        load_path.clear();
    } else {
        retro::logInfo("Loading default configuration: {}", config_path);
        config_path = retro_save_directory / (retro_library_name + ".conf");
        if (extension == ".iso" || extension == ".cue") {
            disk_load_image = std::move(load_path);
            load_path.clear();
        }
    }

    if (game_path.has_parent_path()) {
        try {
            load_game_directory = std::filesystem::absolute(game_path.parent_path());
        }
        catch (const std::filesystem::filesystem_error& e)
        {
            retro::logWarn("Failed to get absolute path of content directory: {}", e.what());
        }
    }

    emu_thread = std::thread(start_dosbox, load_path.u8string());
    // Run dosbox until it sets its initial video mode.
    while (switchThread() != ThreadSwitchReason::VideoModeChange)
        ;
    currentWidth = RDOSGFXwidth;
    currentHeight = RDOSGFXheight;
    current_aspect_ratio = dosbox_aspect_ratio;
    if (run_synced) {
        currentFPS = render.src.fps;
    }
    update_mouse_speed_fix();

    if (!disk_load_image.empty()) {
        disk_control::mount(std::move(disk_load_image));
    }

    update_core_option_visibility();
    return true;
}

auto retro_load_game_special(
    const unsigned /*game_type*/, const retro_game_info* const /*info*/, const size_t /*num_info*/)
    -> bool
{
    return false;
}

static void upload_audio(const int16_t* src, int len_frames) noexcept
{
    while (len_frames > 0) {
        const auto uploaded_frames = audio_batch_cb(src, len_frames);
        src += uploaded_frames * 2;
        len_frames -= uploaded_frames;
    }
}

static void checkVideoModeChange() noexcept
{
    if (run_synced && currentFPS != render.src.fps && render.src.fps != 0) {
        update_gfx_mode(true);
    } else if (
        dosbox_aspect_ratio != current_aspect_ratio || RDOSGFXwidth != currentWidth
        || RDOSGFXheight != currentHeight)
    {
        update_gfx_mode(false);
    }
}

void retro_run()
{
    if (dosbox_exit) {
        if (emu_thread.joinable()) {
            switchThread();
            emu_thread.join();
        }
        environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, nullptr);
        return;
    }

    bool fast_forward = false;
    environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fast_forward);
    DOSBOX_UnlockSpeed(fast_forward);

    if (retro::core_options.changed()) {
        check_variables();
        update_core_option_visibility();
        MAPPER_Init();
    }

    /* Once C is mounted, mount the overlay */
    if (Drives['C' - 'A'] && mount_overlay) {
        auto overlay_directory =
            retro_save_directory / retro_library_name / game_path.parent_path().filename();
        mount_overlay_filesystem('C', std::move(overlay_directory));
        mount_overlay = false;
    }

    /* Read input */
    MAPPER_Run(false);

    /* Run emulator */
    fakeTimingReset();
    while (switchThread() == ThreadSwitchReason::VideoModeChange) {
        checkVideoModeChange();
    }

    /* Virtual keyboard */
    if (retro_vkbd)
        print_vkbd();

    // If we have a new frame, submit it.
    if (dosbox_frontbuffer_uploaded && use_frame_duping) {
        video_cb(nullptr, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else if (run_synced) {
        video_cb(dosbox_framebuffers[0].data(), RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else {
        video_cb(dosbox_frontbuffer->data(), RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    }
    dosbox_frontbuffer_uploaded = true;

    if (run_synced) {
        retro_audio_buffer_frames = queue_audio();
    }
    upload_audio(retro_audio_buffer.data(), retro_audio_buffer_frames);
    retro_audio_buffer_frames = 0;

    if (use_retro_midi && have_retro_midi && retro_midi_interface.output_enabled()) {
        retro_midi_interface.flush();
    }
}

void retro_reset()
{
    restart_program(control->startup_params);
}

auto retro_get_memory_data(const unsigned type) -> void*
{
    if (type == RETRO_MEMORY_SYSTEM_RAM) {
        return GetMemBase();
    }
    return nullptr;
}

auto retro_get_memory_size(const unsigned type) -> size_t
{
    if (type == RETRO_MEMORY_SYSTEM_RAM) {
        return MEM_TotalPages() * 4096;
    }
    return 0;
}

/* Stubs */
auto retro_serialize_size() -> size_t
{
    return 0;
}

auto retro_serialize(void* const /*data*/, const size_t /*size*/) -> bool
{
    return false;
}

auto retro_unserialize(const void* const /*data*/, const size_t /*size*/) -> bool
{
    return false;
}

void retro_cheat_reset()
{ }

void retro_cheat_set(unsigned /*unused*/, bool /*unused1*/, const char* /*unused2*/)
{ }

void retro_unload_game()
{ }

auto retro_get_region() -> unsigned
{
    return RETRO_REGION_NTSC;
}

/*

Copyright (C) 2002-2018 The DOSBox Team
Copyright (C) 2015-2018 Andrés Suárez
Copyright (C) 2019 Nikos Chantziaras <realnc@gmail.com>

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
