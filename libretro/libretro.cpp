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
#include "libretro_dosbox.h"
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
#ifdef HAVE_LIBNX
    #include <switch.h>
extern "C" Jit dynarec_jit;
#endif

#define RETRO_DEVICE_JOYSTICK RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_ANALOG, 1)

#define CORE_VERSION SVN_VERSION " " GIT_VERSION

#ifdef WITH_FAKE_SDL
bool startup_state_capslock;
bool startup_state_numlock;
#endif

bool autofire;
static bool dosbox_initialiazed = false;

std::set<std::string> locked_dosbox_variables;

Bit32u MIXER_RETRO_GetFrequency();
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

/* core option variables */
bool run_synced = true;
static bool use_spinlock = false;

/* directories */
std::filesystem::path retro_save_directory;
std::filesystem::path retro_system_directory;
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
static uint8_t audioData[829 * 4]; // 49716hz max
static uint32_t samplesPerFrame = 735;
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

    if (locked_dosbox_variables.count(var_string) != 0
        && retro::core_options["option_handling"].toString() == "disable changed")
    {
        return false;
    }

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
    retro::logDebug("Variable {}::{} updated to {}.", section_string, var_string, val_string);
    return ret;
}

static void leave_thread(const Bitu /*val*/)
{
    MIXER_CallBack(nullptr, audioData, samplesPerFrame * 4);
    switchThread();

    if (!run_synced) {
        /* Schedule the next frontend interrupt */
        PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
    }
}

static void update_gfx_mode(const bool change_fps)
{
    const float old_fps = currentFPS;
    retro_system_av_info new_av_info;
    bool cb_error = false;
    retro_get_system_av_info(&new_av_info);

    new_av_info.geometry.base_width = RDOSGFXwidth;
    new_av_info.geometry.base_height = RDOSGFXheight;
    new_av_info.geometry.aspect_ratio = dosbox_aspect_ratio;

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
            "Resolution changed {}x{} @ {:.3}Hz AR: {:.5} => {}x{} @ {:.3}Hz AR: {:.5}.",
            currentWidth, currentHeight, old_fps, current_aspect_ratio, RDOSGFXwidth, RDOSGFXheight,
            currentFPS, dosbox_aspect_ratio);
    }

    currentWidth = RDOSGFXwidth;
    currentHeight = RDOSGFXheight;
    current_aspect_ratio = dosbox_aspect_ratio;
}

static RETRO_CALLCONV auto update_core_option_visibility() -> bool
{
    using namespace retro;

    bool updated = false;
    const bool show_all = core_options["adv_options"].toBool();

    const auto& mode = core_options["cpu_cycles_mode"].toString();
    updated |= core_options.setVisible("cpu_cycles_limit", mode == "max" || mode == "auto");
    updated |= core_options.setVisible(
        {"cpu_cycles_multiplier_realmode", "cpu_cycles_realmode",
         "cpu_cycles_multiplier_fine_realmode", "cpu_cycles_fine_realmode"},
        mode == "auto");

    const bool mpu_enabled = core_options["mpu_type"].toString() != "none";
    updated |= core_options.setVisible("midi_driver", mpu_enabled);

    const auto& midi_driver = core_options["midi_driver"].toString();
#ifdef WITH_BASSMIDI
    const auto bassmidi_enabled = mpu_enabled && midi_driver == "bassmidi";
    updated |=
        core_options.setVisible({"bassmidi.soundfont", "bassmidi.sfvolume"}, bassmidi_enabled);
    updated |= core_options.setVisible("bassmidi.voices", show_all && bassmidi_enabled);
#endif

#ifdef WITH_FLUIDSYNTH
    const auto fsynth_enabled = mpu_enabled && midi_driver == "fluidsynth";
    for (const auto* name : {"fluid.soundfont", "fluid.gain", "fluid.polyphony", "fluid.cores"}) {
        updated |= core_options.setVisible(name, fsynth_enabled);
    }
    for (const auto* name :
         {"fluid.samplerate", "fluid.reverb", "fluid.reverb.roomsize", "fluid.reverb.damping",
          "fluid.reverb.width", "fluid.reverb.level", "fluid.chorus", "fluid.chorus.number",
          "fluid.chorus.level", "fluid.chorus.speed", "fluid.chorus.depth"})
    {
        updated |= core_options.setVisible(name, show_all && fsynth_enabled);
    }
#endif

    const auto mt32_enabled = mpu_enabled && midi_driver == "mt32";
    for (const auto* name : {"mt32.type", "mt32.thread", "mt32.partials", "mt32.analog"}) {
        updated |= core_options.setVisible(name, mt32_enabled);
    }
    for (const auto* name :
         {"mt32.reverse.stereo", "mt32.dac", "mt32.reverb.mode", "mt32.reverb.time",
          "mt32.reverb.level", "mt32.rate", "mt32.src.quality", "mt32.niceampramp"})
    {
        updated |= core_options.setVisible(name, show_all && mt32_enabled);
    }
    const bool mt32_is_threaded = core_options["mt32.thread"].toBool();
    for (const auto* name : {"mt32.chunk", "mt32.prebuffer"}) {
        updated |= core_options.setVisible(name, show_all && mt32_enabled && mt32_is_threaded);
    }

#ifdef HAVE_ALSA
    updated |= core_options.setVisible("midi_port", midi_driver == "alsa" && mpu_enabled);
#endif
#ifdef __WIN32__
    updated |= core_options.setVisible("midi_port", midi_driver == "win32" && mpu_enabled);
#endif

#ifdef WITH_VOODOO
    updated |=
        core_options.setVisible("voodoo_memory_size", core_options["voodoo"].toString() != "false");
#endif

    const auto& machine_type = core_options["machine_type"].toString();

    updated |= core_options.setVisible(
        {"machine_cga_composite_mode", "machine_cga_model"}, show_all && machine_type == "cga");

    updated |=
        core_options.setVisible("machine_hercules_palette", show_all && machine_type == "hercules");

    const auto& sb_type = core_options["sblaster_type"].toString();
    const bool sb_enabled = sb_type != "none";
    const bool sb_is_gameblaster = sb_type == "gb";
    const bool sb_is_sb16 = sb_type == "sb16";
    updated |= core_options.setVisible(
        {"sblaster_base", "sblaster_opl_mode", "sblaster_opl_emu"}, show_all && sb_enabled);
    updated |= core_options.setVisible(
        {"sblaster_irq", "sblaster_dma"}, show_all && sb_enabled && !sb_is_gameblaster);
    updated |= core_options.setVisible("sblaster_hdma", show_all && sb_is_sb16);

    auto gus_enabled = core_options["gus"].toBool();
    updated |= core_options.setVisible({"gusbase", "gusirq", "gusdma"}, show_all && gus_enabled);

    updated |= core_options.setVisible(
        {"default_mount_freesize", "thread_sync", "cpu_type", "scaler", "mpu_type", "tandy",
         "disney", "log_method", "log_level"},
        show_all);

#ifdef WITH_PINHACK
    const auto pinhack_enabled = core_options["pinhack"].toBool();
    const bool pinhack_expand_height_enabled =
        core_options["pinhackexpandheight_coarse"].toInt() != 0;
    updated |= core_options.setVisible(
        {"pinhackactive", "pinhacktriggerwidth", "pinhacktriggerheight",
         "pinhackexpandheight_coarse"},
        pinhack_enabled);
    updated |= core_options.setVisible(
        "pinhackexpandheight_fine", pinhack_enabled && pinhack_expand_height_enabled);
#endif

    static const std::regex pad_map_regex{"^pad(0|1)_map_"};
    core_options.setVisible(pad_map_regex, core_options["show_kb_map_options"].toBool());

    return updated;
}

static void check_blaster_variables(const bool autoexec)
{
    using namespace retro;

    update_dosbox_variable(
        autoexec, "sblaster", "sbtype", core_options["sblaster_type"].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "sbbase", core_options["sblaster_base"].toString());
    update_dosbox_variable(autoexec, "sblaster", "irq", core_options["sblaster_irq"].toString());
    update_dosbox_variable(autoexec, "sblaster", "dma", core_options["sblaster_dma"].toString());
    update_dosbox_variable(autoexec, "sblaster", "hdma", core_options["sblaster_hdma"].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplmode", core_options["sblaster_opl_mode"].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplemu", core_options["sblaster_opl_emu"].toString());
}

static void check_gus_variables(const bool autoexec)
{
    using namespace retro;

    update_dosbox_variable(autoexec, "gus", "gus", core_options["gus"].toString());
    update_dosbox_variable(autoexec, "gus", "gusbase", core_options["gusbase"].toString());
    update_dosbox_variable(autoexec, "gus", "gusirq", core_options["gusirq"].toString());
    update_dosbox_variable(autoexec, "gus", "gusdma", core_options["gusdma"].toString());
}

static void check_vkbd_variables()
{
    using namespace retro;

    const auto& theme = core_options["vkbd_theme"].toString();
    if (theme.find("light") != std::string::npos) {
        opt_vkbd_theme = 1;
    } else if (theme.find("dark") != std::string::npos) {
        opt_vkbd_theme = 2;
    }
    if (theme.find("outline") != std::string::npos) {
        opt_vkbd_theme |= 0x80;
    }

    const auto& transparency = core_options["vkbd_transparency"].toString();
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
         {"pinhack", "pinhackactive", "pinhacktriggerwidth", "pinhacktriggerheight"}) {
        updated |= update_dosbox_variable(false, "pinhack", name, core_options[name].toString());
    }

    const int expand_height_coarse = core_options["pinhackexpandheight_coarse"].toInt();
    if (expand_height_coarse > 0) {
        const int expand_height_fine = core_options["pinhackexpandheight_fine"].toInt();
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

static void check_cpu_cycle_variables()
{
    using namespace retro;

    const auto& mode = core_options["cpu_cycles_mode"].toString();
    const int realmode_cycles = core_options["cpu_cycles_realmode"].toInt()
            * core_options["cpu_cycles_multiplier_realmode"].toInt()
        + core_options["cpu_cycles_fine_realmode"].toInt()
            * core_options["cpu_cycles_multiplier_fine_realmode"].toInt();
    const int cycles =
        core_options["cpu_cycles"].toInt() * core_options["cpu_cycles_multiplier"].toInt()
        + core_options["cpu_cycles_fine"].toInt()
            * core_options["cpu_cycles_multiplier_fine"].toInt();
    const auto cycles_str = std::to_string(cycles);
    const std::string max_limits_str = [cycles, &cycles_str] {
        const auto& limit = core_options["cpu_cycles_limit"].toString();
        std::string retval;
        if (limit != "none") {
            retval += ' ' + limit;
        }
        if (cycles > 0) {
            retval += " limit " + cycles_str;
        }
        return retval;
    }();

    std::string output = mode;
    if (mode == "fixed" && cycles > 0) {
        output += ' ' + cycles_str;
    } else if (mode == "max") {
        output += max_limits_str;
    } else {
        if (realmode_cycles > 0) {
            output += ' ' + std::to_string(realmode_cycles);
        }
        output += max_limits_str;
    }
    update_dosbox_variable(false, "cpu", "cycles", output);
}

static void update_bassmidi_variables()
{
#ifdef WITH_BASSMIDI
    const auto soundfont = retro_system_directory / "soundfonts"
        / retro::core_options["bassmidi.soundfont"].toString();
    update_dosbox_variable(false, "bassmidi", "bassmidi.soundfont", soundfont.u8string());

    for (const auto* name : {"bassmidi.sfvolume", "bassmidi.voices"}) {
        update_dosbox_variable(false, "bassmidi", name, retro::core_options[name].toString());
    }
#endif
}

static void update_fsynth_variables()
{
#ifdef WITH_FLUIDSYNTH
    const auto soundfont =
        retro_system_directory / "soundfonts" / retro::core_options["fluid.soundfont"].toString();
    update_dosbox_variable(false, "midi", "fluid.soundfont", soundfont.u8string());

    for (const auto* name :
         {"fluid.gain", "fluid.polyphony", "fluid.cores", "fluid.samplerate", "fluid.reverb",
          "fluid.reverb.roomsize", "fluid.reverb.damping", "fluid.reverb.width",
          "fluid.reverb.level", "fluid.chorus", "fluid.chorus.number", "fluid.chorus.level",
          "fluid.chorus.speed", "fluid.chorus.depth"})
    {
        update_dosbox_variable(false, "midi", name, retro::core_options[name].toString());
    }
#endif
}

static void update_mt32_variables()
{
    update_dosbox_variable(false, "midi", "mt32.romdir", retro_system_directory.u8string());

    for (const auto* name :
         {"mt32.type", "mt32.thread", "mt32.partials", "mt32.analog", "mt32.reverse.stereo",
          "mt32.dac", "mt32.reverb.mode", "mt32.reverb.time", "mt32.reverb.level", "mt32.rate",
          "mt32.src.quality", "mt32.niceampramp", "mt32.chunk", "mt32.prebuffer"})
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
    if (retro::core_options["log_method"].toString() == "frontend") {
        use_libretro_log_cb();
    } else {
        retro::setRetroLogCb(log_cb.log);
    }
}

static void update_log_verbosity()
{
    const auto& level = retro::core_options["log_level"].toString();
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
        run_synced = core_options["core_timing"].toString() == "external";

        if (dosbox_initialiazed && run_synced != old_timing) {
            if (!run_synced) {
                PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
            }
            update_gfx_mode(true);
        }
    }

    use_spinlock = core_options["thread_sync"].toString() == "spin";
    useSpinlockThreadSync(use_spinlock);

    if (core_options["option_handling"].toString() == "all off") {
        return;
    }

    if (!dosbox_initialiazed) {
        update_dosbox_variable(false, "dosbox", "memsize", core_options["memory_size"].toString());

        svgaCard = SVGA_None;
        machine = MCH_VGA;
        int10.vesa_nolfb = false;
        int10.vesa_oldvbe = false;
        const std::string& machine_type = core_options["machine_type"].toString();
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
        update_dosbox_variable(false, "mixer", "blocksize", core_options["blocksize"].toString());
        update_dosbox_variable(false, "mixer", "prebuffer", core_options["prebuffer"].toString());

#ifdef WITH_VOODOO
        update_dosbox_variable(false, "pci", "voodoo", core_options["voodoo"].toString());
        update_dosbox_variable(
            false, "pci", "voodoomem", core_options["voodoo_memory_size"].toString());
#endif

        mount_overlay = core_options["save_overlay"].toBool();
    } else {
        if (machine == MCH_HERC) {
            herc_pal = core_options["machine_hercules_palette"].toInt();
            Herc_Palette();
            VGA_DAC_CombineColor(1, 7);
        } else if (machine == MCH_CGA) {
            CGA_Composite_Mode(core_options["machine_cga_composite_mode"].toInt());
            CGA_Model(core_options["machine_cga_model"].toInt());
        }

        check_blaster_variables(false);
        check_gus_variables(false);

        {
            const bool prev_force_2axis_joystick = force_2axis_joystick;
            const int prev_mouse_emu_deadzone = mouse_emu_deadzone;

            force_2axis_joystick = core_options["joystick_force_2axis"].toBool();
            mouse_emu_deadzone = core_options["emulated_mouse_deadzone"].toInt();
            if (prev_force_2axis_joystick != force_2axis_joystick
                || prev_mouse_emu_deadzone != mouse_emu_deadzone)
            {
                MAPPER_Init();
            }
        }

        mouse_speed_factor_x = core_options["mouse_speed_factor_x"].toFloat();
        mouse_speed_factor_y = core_options["mouse_speed_factor_y"].toFloat();

        update_dosbox_variable(false, "cpu", "cputype", core_options["cpu_type"].toString());
        update_dosbox_variable(false, "cpu", "core", core_options["cpu_core"].toString());
        update_dosbox_variable(
            false, "render", "aspect", core_options["aspect_correction"].toString());
        update_dosbox_variable(false, "render", "scaler", core_options["scaler"].toString());
        update_dosbox_variable(
            false, "joystick", "timed", core_options["joystick_timed"].toString());

        check_cpu_cycle_variables();

        update_dosbox_variable(false, "speaker", "pcspeaker", core_options["pcspeaker"].toString());

        {
            const auto& mpu_type = core_options["mpu_type"].toString();
            update_dosbox_variable(false, "midi", "mpu401", mpu_type);

            const auto& midi_driver = core_options["midi_driver"].toString();
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
            const auto& current_value = core_options["midi_port"].toString();
            for (const auto& [port, client, port_name] : alsa_midi_ports) {
                if (client + ':' + port_name == current_value) {
                    update_dosbox_variable(false, "midi", "midiconfig", port);
                    break;
                }
            }
#endif
#ifdef __WIN32__
            update_dosbox_variable(
                false, "midi", "midiconfig", core_options["midi_port"].toString());
#endif
        }

#if defined(C_IPX)
        update_dosbox_variable(false, "ipx", "ipx", core_options["ipx"].toString());
#endif

        update_dosbox_variable(false, "speaker", "tandy", core_options["tandy"].toString());

        if (!dosbox_initialiazed) {
            const auto& disney_val = core_options["disney"].toString();
            update_dosbox_variable(false, "speaker", "disney", disney_val);
            disney_init = disney_val == "on";
        }

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
    info->geometry.base_width = 320;
    info->geometry.base_height = 200;
    info->geometry.max_width = GFX_MAX_WIDTH;
    info->geometry.max_height = GFX_MAX_HEIGHT;
    info->geometry.aspect_ratio = 4.0 / 3;
    info->timing.fps = currentFPS;
    info->timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
}

void retro_init()
{
    use_libretro_log_cb();

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
    // Add values to the midi port option. We don't use numerical ports since these can change. We
    // instead use the MIDI client and port name and resolve them back to numerical ports later on.
    {
        std::vector<retro::CoreOptionValue> values;
        for (const auto& [port, client, port_name] : alsa_midi_ports) {
            values.emplace_back(
                client + ':' + port_name, "[" + client + "] " + port_name + " - " + port);
        }
        if (values.empty()) {
            values.emplace_back("none", "(no MIDI ports found)");
        }
        retro::core_options.option("midi_port")->setValues(values, values.front());
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
        retro::core_options.option("midi_port")->setValues(values, values.front());
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
        retro::core_options.option("bassmidi.soundfont")
            ->setValues(bass_values, bass_values.front());
#endif
#ifdef WITH_FLUIDSYNTH
        retro::core_options.option("fluid.soundfont")
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

#ifdef HAVE_LIBNX
    jitClose(&dynarec_jit);
#endif

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
#ifndef HAVE_LIBNX
        config_path = std::filesystem::absolute(load_path);
#else
        config_path = load_path;
#endif
        load_path.clear();
    } else {
        retro::logInfo("Loading default configuration: {}", config_path);
        config_path = retro_save_directory / (retro_library_name + ".conf");
        if (extension == ".iso" || extension == ".cue") {
            disk_load_image = std::move(load_path);
            load_path.clear();
        }
    }

    // Change the current working directory so that it's possible to have paths in .conf and
    // .bat files (like MOUNT commands) that are relative to the content directory.
    if (game_path.has_parent_path()) {
        try {
            std::filesystem::current_path(game_path.parent_path());
        }
        catch (const std::exception& e) {
            retro::logError(
                "Failed to change current directory to \"{}\": {}", game_path, e.what());
        }
    }

    emu_thread = std::thread(start_dosbox, load_path.u8string());
    switchThread();

    if (!disk_load_image.empty()) {
        disk_control::mount(std::move(disk_load_image));
    }

    samplesPerFrame = MIXER_RETRO_GetFrequency() / currentFPS;
    return true;
}

auto retro_load_game_special(
    const unsigned /*game_type*/, const retro_game_info* const /*info*/, const size_t /*num_info*/)
    -> bool
{
    return false;
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

    /* Dynamic resolution switching */
    if (RDOSGFXwidth != currentWidth || RDOSGFXheight != currentHeight
        || (run_synced && fabs(currentFPS - render.src.fps) > 0.05f && render.src.fps != 0))
    {
        update_gfx_mode(run_synced);
    } else if (dosbox_aspect_ratio != current_aspect_ratio) {
        update_gfx_mode(false);
    }

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
    switchThread();

    /* Virtual keyboard */
    if (retro_vkbd)
        print_vkbd();

    // If we have a new frame, submit it.
    if (dosbox_frontbuffer_uploaded) {
        video_cb(nullptr, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else if (run_synced) {
        video_cb(dosbox_framebuffers[0].data(), RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else {
        video_cb(dosbox_frontbuffer->data(), RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    }
    dosbox_frontbuffer_uploaded = true;

    if (run_synced) {
        MIXER_CallBack(nullptr, audioData, samplesPerFrame * 4);
    }
    /* Upload audio */
    audio_batch_cb((int16_t*)audioData, samplesPerFrame);

    if (use_retro_midi && have_retro_midi && retro_midi_interface.output_enabled()) {
        retro_midi_interface.flush();
    }
    samplesPerFrame = MIXER_RETRO_GetFrequency() / currentFPS;
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
