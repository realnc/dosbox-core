/*
 *  Copyright (C) 2002-2018 - The DOSBox Team
 *  Copyright (C) 2015-2018 - Andrés Suárez
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "CoreOptions.h"
#include "control.h"
#include "dos/cdrom.h"
#include "dos/drives.h"
#include "dosbox.h"
#include "emu_thread.h"
#include "fake_timing.h"
#include "file/file_path.h"
#include "ints/int10.h"
#include "joystick.h"
#include "libretro.h"
#include "libretro_dosbox.h"
#include "mapper.h"
#include "midi_alsa.h"
#include "midi_win32.h"
#include "mixer.h"
#include "pic.h"
#include "programs.h"
#include "render.h"
#include "setup.h"
#ifdef ANDROID
#include "nonlibc.h"
#endif
#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdlib>
#include <filesystem>
#include <memory>
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

#ifndef PATH_MAX_LENGTH
#define PATH_MAX_LENGTH 4096
#endif

#define CORE_VERSION "0.74-SVN"

#ifndef PATH_SEPARATOR
#if defined(WINDOWS_PATH_STYLE) || defined(_WIN32)
#define PATH_SEPARATOR '\\'
#else
#define PATH_SEPARATOR '/'
#endif
#endif

#ifdef WITH_FAKE_SDL
bool startup_state_capslock;
bool startup_state_numlock;
#endif

#ifdef _WIN32
static constexpr char slash = '\\';
#else
static constexpr char slash = '/';
#endif

bool autofire;
static bool dosbox_initialiazed = false;

unsigned mouse_emu_deadzone;
float mouse_speed_factor_x = 1.0;
float mouse_speed_factor_y = 1.0;

Bit32u MIXER_RETRO_GetFrequency();
void MIXER_CallBack(void * userdata, uint8_t *stream, int len);

extern Bit8u herc_pal;
MachineType machine = MCH_VGA;
SVGACards svgaCard = SVGA_None;

/* input variables */
bool gamepad[16]; /* true means gamepad, false means joystick */
bool connected[16];
bool emulated_mouse;

/* core option variables */
bool run_synced = true;
static bool use_spinlock = false;

/* directories */
std::string retro_save_directory;
static std::string retro_system_directory;
static std::string retro_content_directory;
static const std::string retro_library_name = "DOSBox-core";

/* libretro variables */
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t poll_cb;
retro_input_state_t input_cb;
retro_environment_t environ_cb;
retro_log_printf_t log_cb;

/* DOSBox state */
static std::string loadPath;
static std::string gamePath;
static std::string configPath;
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
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }

/* disk-control variables */
static char disk_array[16][PATH_MAX_LENGTH];
static unsigned disk_index = 0;
static unsigned disk_count = 0;
static bool disk_tray_ejected;
static char disk_load_image[PATH_MAX_LENGTH];
static bool mount_overlay = true;

// Thread we run dosbox in.
static std::thread emu_thread;

/* helper functions */
static char last_written_character = 0;

static void write_out_buffer(const char* const format,...)
{
    char buf[2048];
    va_list msg;

    va_start(msg,format);
#ifdef ANDROID
    portable_vsnprintf(buf,2047,format,msg);
#else
    vsnprintf(buf,2047,format,msg);
#endif
    va_end(msg);

    Bit16u size = (Bit16u)strlen(buf);
    dos.internal_output=true;
    for(Bit16u i = 0; i < size;i++) {
        Bit8u out;Bit16u s=1;
        if (buf[i] == 0xA && last_written_character != 0xD) {
            out = 0xD;DOS_WriteFile(STDOUT,&out,&s);
        }
        last_written_character = out = buf[i];
        DOS_WriteFile(STDOUT,&out,&s);
    }
    dos.internal_output=false;
}

static void write_out(const char* const format,...)
{
    write_out_buffer("\n");
    write_out_buffer(format);
}

static void mount_overlay_filesystem(const char drive, const char* const path)
{
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] mounting %s in %c as overlay\n", path, drive);

    if (!Drives[drive - 'A']) {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] base drive %c is not mounted\n", drive);
        write_out("No basedrive mounted yet!");
        return;
    }

    auto* base_drive = dynamic_cast<localDrive*>(Drives[drive - 'A']);
    if (!base_drive || dynamic_cast<cdromDrive*>(base_drive)
        || dynamic_cast<Overlay_Drive*>(base_drive))
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] base drive %c is not compatible\n", drive);
        return;
    }

    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] creating save directory %s\n", path);
    try {
        std::filesystem::create_directory(path);
    }
    catch (const std::exception& e) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[dosbox] error creating overlay directory %s: %s\n", path,
                   e.what());
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
        base_drive->getBasedir(), path, bytes_per_sector, sectors_per_cluster, total_clusters,
        free_clusters, 0xF8, o_error);
    if (o_error) {
        if (o_error == 1 && log_cb) {
            log_cb(RETRO_LOG_INFO, "[dosbox] can't mix absolute and relative paths");
        } else if (o_error == 2 && log_cb) {
            log_cb(RETRO_LOG_INFO, "[dosbox] overlay can't be in the same underlying file system");
        } else if (log_cb){
            log_cb(RETRO_LOG_INFO, "[dosbox] something went wrong");
        }
        return;
    }

    mem_writeb(Real2Phys(dos.tables.mediaid) + (drive - 'A') * 9, overlay->GetMediaByte());
    overlay->dirCache.SetLabel((drive + std::string("_OVERLAY")).c_str(), false, false);
    Drives[drive - 'A'] = overlay.release();
    delete base_drive;
}

static auto mount_disk_image(const char* const path, const bool silent) -> bool
{
    char msg[256];

    if(control->SecureMode())
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] this operation is not permitted in secure mode\n");
        return false;
    }

    std::string extension = strrchr(path, '.');
    if (extension == ".img")
        log_cb(RETRO_LOG_INFO, "[dosbox] mounting disk as floppy %s\n", path);
    else if (extension == ".iso" || extension == ".cue")
        log_cb(RETRO_LOG_INFO, "[dosbox] mounting disk as cdrom %s\n", path);
    else
    {
        log_cb(RETRO_LOG_INFO, "[dosbox] unsupported disk image\n %s", extension.c_str());
        return false;
    }

    if (disk_count == 0)
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] no disks added to index\n");
        return false;
    }

    if (extension == ".img")
    {
        char drive = 'A';

        Bit8u mediaid=0xF0;
        Bit16u sizes[4];
        std::string fstype="floppy";

        std::string str_size;
        std::string label;

        DOS_Drive* floppy = new fatDrive(path, sizes[0],sizes[1],sizes[2],sizes[3],0);
        if(!(dynamic_cast<fatDrive*>(floppy))->created_successfully)
            return false;

        DriveManager::AppendDisk(drive - 'A', floppy);
        DriveManager::InitializeDrive(drive - 'A');

        /* set the correct media byte in the table */
        mem_writeb(Real2Phys(dos.tables.mediaid) + (drive - 'A') * 9, mediaid);

        /* command uses dta so set it to our internal dta */
        dos.dta(dos.tables.tempdta);

        for(Bitu i = 0; i < DOS_DRIVES; i++)
            if (Drives[i]) Drives[i]->EmptyCache();

        DriveManager::CycleDisks(drive - 'A', true);

        snprintf(msg, sizeof(msg), "Drive %c is mounted as %s.\n", drive, path);
        if (!silent) write_out(msg);
            return true;
    }
    else if (extension == ".iso" || extension == ".cue")
    {
        int error = -1;
        char drive = 'D';

        Bit8u mediaid = 0xF8;
        std::string fstype="iso";

        MSCDEX_SetCDInterface(CDROM_USE_SDL, -1);

        DOS_Drive* iso = new isoDrive(drive, path, mediaid, error);
        switch (error) {
            case 0:    break;
            case 1:
                snprintf(msg, sizeof(msg), "MSCDEX: Failure: Drive-letters of multiple CD-ROM drives have to be continuous.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            case 2:
                snprintf(msg, sizeof(msg), "MSCDEX: Failure: Not yet supported.");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            case 3:
                snprintf(msg, sizeof(msg), "MSCDEX: Specified location is not a CD-ROM drive.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            case 4:
                snprintf(msg, sizeof(msg), "MSCDEX: Failure: Invalid file or unable to open.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            case 5:
                snprintf(msg, sizeof(msg), "MSCDEX: Failure: Too many CD-ROM drives (max: 5). MSCDEX Installation failed.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            case 6:
                snprintf(msg, sizeof(msg), "MSCDEX: Mounted subdirectory: limited support.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
            default:
                snprintf(msg, sizeof(msg), "MSCDEX: Failure: Unknown error.\n");
                log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
                if (!silent) write_out(msg);
                    return false;
        }

        DriveManager::AppendDisk(drive - 'A', iso);
        DriveManager::InitializeDrive(drive - 'A');

        /* set the correct media byte in the table */
        mem_writeb(Real2Phys(dos.tables.mediaid) + (drive - 'A') * 9, mediaid);

        for(Bitu i = 0; i < DOS_DRIVES; i++)
            if (Drives[i]) Drives[i]->EmptyCache();

        DriveManager::CycleDisks(drive - 'A', true);

        snprintf(msg, sizeof(msg), "Drive %c is mounted as %s.\n", drive, path);
        log_cb(RETRO_LOG_INFO, "[dosbox] %s", msg);
        if (!silent) write_out(msg);
            return true;
    }
    return false;
}

static auto unmount_disk_image(const char* const path) -> bool
{
    char drive;
    std::string extension = strrchr(path, '.');

    if (disk_count == 0)
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] no disks added to index\n");
        return false;
    }

    if (extension == ".img")
    {
        log_cb(RETRO_LOG_INFO, "[dosbox] unmounting floppy %s\n", path);
        drive = 'A';
    }
    else if (extension == ".iso" || extension == ".cue")
    {
        log_cb(RETRO_LOG_INFO, "[dosbox] umounting cdrom %s\n", path);
        drive = 'D';
    }
    else
    {
        log_cb(RETRO_LOG_INFO, "[dosbox] unsupported disk image\n %s", extension.c_str());
        return false;
    }
    if (Drives[drive - 'A'])
        DriveManager::UnmountDrive(drive - 'A');
    Drives[drive - 'A'] = nullptr;
    DriveManager::CycleDisks(drive - 'A', true);

    return true;
}

static auto compare_dosbox_variable(
        const std::string& section_string, const std::string& var_string,
        const std::string& val_string) -> bool
{
    bool ret = false;
    Section* section = control->GetSection(section_string);
    Section_prop *secprop = static_cast <Section_prop*>(section);
    if (secprop)
        ret =section->GetPropValue(var_string) == val_string;

    return ret;
}

auto update_dosbox_variable(
        const bool autoexec, const std::string& section_string, const std::string& var_string,
        const std::string& val_string) -> bool
{
    bool ret = false;
    if (dosbox_initialiazed)
    {
        if (compare_dosbox_variable(section_string, var_string, val_string))
            return false;
    }

    Section* section = control->GetSection(section_string);
    Section_prop *secprop = static_cast <Section_prop*>(section);
    if (secprop)
    {
        if (!autoexec)
            section->ExecuteDestroy(false);
        std::string inputline = var_string + "=" + val_string;
        ret = section->HandleInputline(inputline.c_str());
        if (!autoexec)
            section->ExecuteInit(false);
    }
    log_cb(RETRO_LOG_INFO, "[dosbox] variable %s::%s updated\n", section_string.c_str(), var_string.c_str(), val_string.c_str());
    return ret;
}

static auto disk_get_num_images() -> unsigned
{
    return disk_count;
}

static auto disk_get_eject_state() -> bool
{
    return disk_tray_ejected;
}

static auto disk_get_image_index() -> unsigned
{
    return disk_index;
}

static auto disk_set_eject_state(const bool ejected) -> bool
{
    if (log_cb && ejected)
        log_cb(RETRO_LOG_INFO, "[dosbox] tray open\n");
    else if (!ejected)
        log_cb(RETRO_LOG_INFO, "[dosbox] tray closed\n");
    disk_tray_ejected = ejected;

    if (disk_count == 0)
        return true;

    if (ejected)
    {
        if(unmount_disk_image(disk_array[disk_get_image_index()]))
            return true;
        else
            return false;
    }
    if (mount_disk_image(disk_array[disk_get_image_index()], true)) {
        return true;
    }
    return false;
}

static auto disk_set_image_index(const unsigned index) -> bool
{
    if (index < disk_get_num_images())
    {
        disk_index = index;
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] disk index %u\n", index);
        return true;
    }
    return false;
}

static auto disk_add_image_index() -> bool
{
    disk_count++;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] disk count %u\n", disk_count);
    return true;
}

static auto disk_replace_image_index(
        const unsigned index, const retro_game_info* const info) -> bool
{
    if (index < disk_get_num_images())
        snprintf(disk_array[index], sizeof(char) * PATH_MAX_LENGTH, "%s", info->path);
    else
        disk_count--;
    return false;
}

static retro_disk_control_callback disk_interface {
    disk_set_eject_state,
    disk_get_eject_state,
    disk_get_image_index,
    disk_set_image_index,
    disk_get_num_images,
    disk_replace_image_index,
    disk_add_image_index,
};

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

    if (change_fps)
    {
        const float new_fps = run_synced ? render.src.fps : default_fps;

        new_av_info.timing.fps = new_fps;
        new_av_info.timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
        cb_error = !environ_cb(RETRO_ENVIRONMENT_SET_SYSTEM_AV_INFO, &new_av_info);
        if (cb_error && log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] SET_SYSTEM_AV_INFO failed\n");
        currentFPS = new_fps;
    }
    else
    {
        cb_error = !environ_cb(RETRO_ENVIRONMENT_SET_GEOMETRY, &new_av_info);
        if (cb_error && log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] SET_GEOMETRY failed\n");
    }

    if (!cb_error && log_cb)
        log_cb (RETRO_LOG_INFO,
                "[dosbox] resolution changed %dx%d @ %.3fHz AR: %.5f => %dx%d @ %.3fHz AR: %.5f\n",
                currentWidth, currentHeight, old_fps, current_aspect_ratio,
                RDOSGFXwidth, RDOSGFXheight, currentFPS, dosbox_aspect_ratio);

    currentWidth = RDOSGFXwidth;
    currentHeight = RDOSGFXheight;
    current_aspect_ratio = dosbox_aspect_ratio;
}

static auto check_blaster_variables(const bool autoexec) -> bool
{
    using namespace retro;

    const auto& sb_type = core_options["sblaster_type"].toString();

    update_dosbox_variable(autoexec, "sblaster", "sbtype", sb_type);
    update_dosbox_variable(
        autoexec, "sblaster", "sbbase", core_options["sblaster_base"].toString());
    update_dosbox_variable(autoexec, "sblaster", "irq", core_options["sblaster_irq"].toString());
    update_dosbox_variable(autoexec, "sblaster", "dma", core_options["sblaster_dma"].toString());
    update_dosbox_variable(autoexec, "sblaster", "hdma", core_options["sblaster_hdma"].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplmode", core_options["sblaster_opl_mode"].toString());
    update_dosbox_variable(
        autoexec, "sblaster", "oplemu", core_options["sblaster_opl_emu"].toString());
    return sb_type != "none";
}

static auto check_gus_variables(const bool autoexec) -> bool
{
    using namespace retro;

    auto gus_value = core_options["gus"];

    update_dosbox_variable(autoexec, "gus", "gus", gus_value.toString());
    update_dosbox_variable(autoexec, "gus", "gusrate", core_options["gusrate"].toString());
    update_dosbox_variable(autoexec, "gus", "gusbase", core_options["gusbase"].toString());
    update_dosbox_variable(autoexec, "gus", "gusirq", core_options["gusirq"].toString());
    update_dosbox_variable(autoexec, "gus", "gusdma", core_options["gusdma"].toString());
    return gus_value.toBool();
}

void core_autoexec()
{
    check_blaster_variables(true);
    check_gus_variables(true);
}

static void check_variables()
{
    using namespace retro;

    static unsigned cycles, cycles_fine, cycles_limit;
    static unsigned cycles_multiplier, cycles_multiplier_fine;
    static bool update_cycles = false;
    std::string cycles_mode;
    std::string machine_type;

    bool blaster = false;
    bool gus = false;

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

    if (!core_options["use_options"].toBool())
        return;

    const auto adv_core_options = core_options["adv_options"].toBool();

    /* save machine type for option hiding purpose */
    machine_type = core_options["machine_type"].toString();

    if (!dosbox_initialiazed)
    {
        update_dosbox_variable(false, "dosbox", "memsize", core_options["memory_size"].toString());

        svgaCard = SVGA_None;
        machine = MCH_VGA;
        int10.vesa_nolfb = false;
        int10.vesa_oldvbe = false;
        if (machine_type == "hercules")
            machine = MCH_HERC;
        else if (machine_type == "cga")
            machine = MCH_CGA;
        else if (machine_type == "pcjr")
            machine = MCH_PCJR;
        else if (machine_type == "tandy")
            machine = MCH_TANDY;
        else if (machine_type == "ega")
            machine = MCH_EGA;
        else if (machine_type == "svga_s3")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
        }
        else if (machine_type == "svga_et4000")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_TsengET4K;
        }
        else if (machine_type == "svga_et3000")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_TsengET3K;
        }
        else if (machine_type == "svga_paradise")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_ParadisePVGA1A;
        }
        else if (machine_type == "vesa_nolfb")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
            int10.vesa_nolfb = true;
        }
        else if (machine_type == "vesa_nolfb")
        {
            machine = MCH_VGA;
            svgaCard = SVGA_S3Trio;
            int10.vesa_oldvbe = true;
        }
        else
        {
            machine = MCH_VGA;
            svgaCard = SVGA_None;
        }
        update_dosbox_variable(false, "dosbox", "machine", machine_type);

        mount_overlay = core_options["save_overlay"].toBool();
    }
    else
    {
        if (machine == MCH_HERC) {
            herc_pal = core_options["machine_hercules_palette"].toInt();
            Herc_Palette();
            VGA_DAC_CombineColor(1,7);
        } else if (machine == MCH_CGA) {
            CGA_Composite_Mode(core_options["machine_cga_composite_mode"].toInt());
            CGA_Model(core_options["machine_cga_model"].toInt());
        }

        blaster = check_blaster_variables(false);
        gus = check_gus_variables(false);

        {
            const bool prev = emulated_mouse;
            emulated_mouse = core_options["emulated_mouse"].toBool();
            if (prev != emulated_mouse)
                MAPPER_Init();
        }

        {
            const unsigned prev = mouse_emu_deadzone;
            mouse_emu_deadzone = core_options["emulated_mouse_deadzone"].toInt();
            if (prev != mouse_emu_deadzone)
                MAPPER_Init();
        }

        mouse_speed_factor_x = core_options["mouse_speed_factor_x"].toFloat();
        mouse_speed_factor_y = core_options["mouse_speed_factor_y"].toFloat();

        cycles_mode = core_options["cpu_cycles_mode"].toString();
        cycles_limit = core_options["cpu_cycles_limit"].toInt();
        cycles = core_options["cpu_cycles"].toInt();
        cycles_multiplier = core_options["cpu_cycles_multiplier"].toInt();
        cycles_fine = core_options["cpu_cycles_fine"].toInt();
        cycles_multiplier_fine = core_options["cpu_cycles_multiplier_fine"].toInt();
        update_cycles = true;

        update_dosbox_variable(false, "cpu", "cputype", core_options["cpu_type"].toString());
        update_dosbox_variable(false, "cpu", "core", core_options["cpu_core"].toString());
        update_dosbox_variable(
            false, "render", "aspect", core_options["aspect_correction"].toString());
        update_dosbox_variable(false, "render", "scaler", core_options["scaler"].toString());
        update_dosbox_variable(
            false, "joystick", "timed", core_options["joystick_timed"].toString());

        if (update_cycles)
        {
            if (cycles_mode == "fixed")
            {
                char s[32];
                snprintf(s, sizeof(s), "%d", cycles * cycles_multiplier + cycles_fine * cycles_multiplier_fine);
                update_dosbox_variable(false, "cpu", "cycles", s);
            }
            else if (cycles_mode == "max")
            {
                char s[32];
                snprintf(s, sizeof(s), "%s %d%%", cycles_mode.c_str(), cycles_limit);
                update_dosbox_variable(false, "cpu", "cycles", s);
            }
            else
                update_dosbox_variable(false, "cpu", "cycles", cycles_mode);

            update_cycles = false;
        }

        update_dosbox_variable(false, "speaker", "pcspeaker", core_options["pcspeaker"].toString());

        {
            const auto& mpu_type = core_options["mpu_type"].toString();
            update_dosbox_variable(false, "midi", "mpu401", mpu_type);
            core_options.setVisible("midi_driver", mpu_type != "none");

            const auto& midi_driver = core_options["midi_driver"].toString();
            use_retro_midi = midi_driver == "libretro";
            update_dosbox_variable(false, "midi", "mididevice", use_retro_midi ? "none" : midi_driver);
            if (use_retro_midi && !have_retro_midi) {
                have_retro_midi = environ_cb(RETRO_ENVIRONMENT_GET_MIDI_INTERFACE, &retro_midi_interface);
                if (log_cb)
                    log_cb(RETRO_LOG_INFO, "[dosbox] libretro MIDI interface %s.\n",
                           have_retro_midi ? "initialized" : "unavailable");
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
            core_options.setVisible("midi_port", midi_driver == "alsa" && mpu_type != "none");
        #endif
        #ifdef __WIN32__
            update_dosbox_variable(
                false, "midi", "midiconfig", core_options["midi_port"].toString());
            core_options.setVisible("midi_port", midi_driver == "win32" && mpu_type != "none");
        #endif
        }

    #if defined(C_IPX)
        update_dosbox_variable(false, "ipx", "ipx", core_options["ipx"].toString());
    #endif

        update_dosbox_variable(false, "speaker", "tandy", core_options["tandy"].toString());

        if (!dosbox_initialiazed)
        {
            const auto& disney_val = core_options["disney"].toString();
            update_dosbox_variable(false, "speaker", "disney", disney_val);
            disney_init = disney_val == "on";
        }
    }

    /* show cycles adjustment if core timing is external or cycle mode is fixed */
    core_options.setVisible(
        {"cpu_cycles", "cpu_cycles_multiplier", "cpu_cycles_fine", "cpu_cycles_multiplier_fine"},
        cycles_mode == "fixed");

    /* show cycles max adjustment if core cycle mode is max */
    core_options.setVisible("cpu_cycles_limit", cycles_mode == "max");

    /* show cga options only if machine is cga and advanced options is enabled */
    core_options.setVisible(
        {"machine_cga_composite_mode", "machine_cga_model"},
        adv_core_options && machine_type == "cga");

    /* show hercules options only if machine is hercules and advanced options is enabled */
    core_options.setVisible(
        "machine_hercules_palette", adv_core_options && machine_type == "hercules");

    /* show blaster options only if soundblaster is enabled and advanced options is enabled */
    core_options.setVisible(
        {"sblaster_base", "sblaster_irq", "sblaster_dma", "sblaster_hdma", "sblaster_opl_mode",
         "sblaster_opl_emu"}, adv_core_options && blaster);

    /* show ultrasound options only if it's it enabled and advanced options is enabled */
    core_options.setVisible({"gusrate", "gusbase", "gusirq", "gusdma"}, adv_core_options && gus);

    /* show these only if advanced options is enabled */
    core_options.setVisible(
        {"thread_sync", "cpu_type", "scaler", "mpu_type", "tandy", "disney"}, adv_core_options);
}

static void start_dosbox()
{
    const char* const argv[2] = {"dosbox", loadPath.c_str()};
    CommandLine com_line(loadPath.empty() ? 1 : 2, argv);
    Config myconf(&com_line);
    control = &myconf;
    dosbox_initialiazed = false;

    /* Init the configuration system and add default values */
    DOSBOX_Init();

    /* Load config */
    if(!configPath.empty())
        control->ParseConfigFile(configPath.c_str());

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

    try
    {
        control->StartUp();
    }
    catch (const EmuThreadCanceled&)
    {
        if (log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] frontend asked to exit\n");
        return;
    }

    if (log_cb)
        log_cb(RETRO_LOG_WARN, "[dosbox] core asked to exit\n");

    dosbox_exit = true;
    switchThread();
}

static void restart_program(const std::vector<std::string>& /*parameters*/)
{
#if 1
    if (log_cb)
        log_cb(RETRO_LOG_WARN, "[dosbox] program restart not supported\n");
#else
    /* TO-DO: this kinda works but it's still not working 100% hence the early return*/
    if(emuThread)
    {
        /* If the frontend wants to exit we need to let the emulator
           run to finish its job. */
        if(frontend_exit)
            co_switch(emuThread);

        co_delete(emuThread);
        emuThread = NULL;
    }

    dosbox_initialiazed = false;
    init_threads();
#endif
}

std::string normalize_path(const std::string& aPath)
{
    std::string result = aPath;
    for(size_t found = result.find_first_of("\\/"); std::string::npos != found; found = result.find_first_of("\\/", found + 1))
    {
        result[found] = PATH_SEPARATOR;
    }

    return result;
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
    cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &disk_interface);

    static const retro_controller_description ports_default[] {
        { "Keyboard + Mouse",    RETRO_DEVICE_KEYBOARD },
        { "Gamepad",             RETRO_DEVICE_JOYPAD },
        { "Joystick",            RETRO_DEVICE_JOYSTICK },
        { "Disconnected",        RETRO_DEVICE_NONE },
        {},
    };
    static const retro_controller_description ports_keyboard[] {
        { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
        { "Disconnected",     RETRO_DEVICE_NONE },
        {},
    };

    static retro_controller_info ports[] {
        { ports_default,  4 },
        { ports_default,  4 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        {},
    };
    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, ports);
}

void retro_set_controller_port_device(const unsigned port, const unsigned device)
{
    connected[port] = false;
    gamepad[port] = false;
    switch (device)
    {
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
#if defined(GIT_VERSION) && defined(SVN_VERSION)
    info->library_version = CORE_VERSION SVN_VERSION GIT_VERSION;
#elif defined(GIT_VERSION)
    info->library_version = CORE_VERSION GIT_VERSION;
#else
    info->library_version = CORE_VERSION;
#endif
    info->valid_extensions = "exe|com|bat|conf|cue|iso|img";
    info->need_fullpath = true;
    info->block_extract = false;
}

void retro_get_system_av_info(retro_system_av_info* const info)
{
    info->geometry.base_width = 320;
    info->geometry.base_height = 200;
    info->geometry.max_width = 1024;
    info->geometry.max_height = 768;
    info->geometry.aspect_ratio = 4.0 / 3;
    info->timing.fps = currentFPS;
    info->timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
}

void retro_init()
{
    /* Initialize logger interface */
    retro_log_callback log;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;
    else
        log_cb = nullptr;

    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] logger interface initialized\n");

    RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RDOSGFXcolorMode);

    const char* system_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
        retro_system_directory = system_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] SYSTEM_DIRECTORY: %s\n", retro_system_directory.c_str());

    const char* save_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
        retro_save_directory = save_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] SAVE_DIRECTORY: %s\n", retro_save_directory.c_str());

    const char* content_dir = nullptr;
    if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
        retro_content_directory = content_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] CONTENT_DIRECTORY: %s\n", retro_content_directory.c_str());

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

    retro::core_options.setEnvironmentCallback(environ_cb);
    retro::core_options.updateFrontend();
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
}

auto retro_load_game(const retro_game_info* const game) -> bool
{
    if (game) {
        // Copy the game path.
        loadPath = normalize_path(game->path);
        const size_t lastDot = loadPath.find_last_of('.');
        char tmp[PATH_MAX_LENGTH];
        snprintf(tmp, sizeof(tmp), "%s", game->path);
        gamePath = std::string(tmp);

        // Find any config file to load.
        if (std::string::npos != lastDot) {
            std::string extension = loadPath.substr(lastDot + 1);
            std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

            if(extension == "conf") {
                configPath = loadPath;
                loadPath.clear();
            } else if(extension == "iso" || extension == "cue") {
                configPath = normalize_path(retro_save_directory + slash + retro_library_name + ".conf");
                if (log_cb) {
                    log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
                }
                disk_add_image_index();
                snprintf(disk_load_image, sizeof(disk_load_image), "%s", loadPath.c_str());
                loadPath.clear();
            } else if(configPath.empty()) {
                configPath = normalize_path(retro_save_directory + slash + retro_library_name + ".conf");
                if (log_cb) {
                    log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
                }
            }
        }
    } else {
        configPath = normalize_path(retro_save_directory + slash +  retro_library_name + ".conf");
        if (log_cb) {
            log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
        }
    }

    emu_thread = std::thread(start_dosbox);
    switchThread();

    samplesPerFrame = MIXER_RETRO_GetFrequency() / currentFPS;
    return true;
}

auto retro_load_game_special(
        const unsigned /*game_type*/, const retro_game_info* const /*info*/,
        const size_t /*num_info*/) -> bool
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

    if (disk_load_image[0]!='\0')
    {
        mount_disk_image(disk_load_image, false);
        snprintf(disk_array[0], sizeof(char) * PATH_MAX_LENGTH, "%s", disk_load_image);
        disk_load_image[0] = '\0';
    }

    /* Dynamic resolution switching */
    if (RDOSGFXwidth != currentWidth || RDOSGFXheight != currentHeight ||
        (run_synced && fabs(currentFPS - render.src.fps) > 0.05f && render.src.fps != 0))
    {
        update_gfx_mode(run_synced);
    }
    else if (dosbox_aspect_ratio != current_aspect_ratio)
    {
        update_gfx_mode(false);
    }

    if (retro::core_options.changed()) {
        check_variables();
    }

    /* Once C is mounted, mount the overlay */
    if (Drives['C' - 'A'] && mount_overlay) {
        std::size_t last_slash = gamePath.find_last_of("/\\");
        std::string s1 = gamePath.substr(0, last_slash);
        std::size_t second_to_last_slash = s1.find_last_of("/\\");
        std::string s2 = s1.substr(second_to_last_slash + 1, last_slash);

        std::string save_directory = retro_save_directory + slash + s2 + slash;
        normalize_path(save_directory);
        mount_overlay_filesystem('C', save_directory.c_str());
        mount_overlay = false;
    }

    /* Read input */
    MAPPER_Run(false);

    /* Run emulator */
    fakeTimingReset();
    switchThread();

    // If we have a new frame, submit it.
    if (dosbox_frontbuffer_uploaded) {
        video_cb(nullptr, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else if (run_synced) {
        video_cb(dosbox_framebuffers[0], RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    } else {
        video_cb(dosbox_frontbuffer, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
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

/* Stubs */
auto retro_get_memory_data(const unsigned /*type*/) -> void*
{ return nullptr; }

auto retro_get_memory_size(const unsigned /*type*/) -> size_t
{ return 0; }

auto retro_serialize_size() -> size_t
{ return 0; }

auto retro_serialize(void* const /*data*/, const size_t /*size*/) -> bool
{ return false; }

auto retro_unserialize(const void* const /*data*/, const size_t /*size*/) -> bool
{ return false; }

void retro_cheat_reset()
{}

void retro_cheat_set(unsigned /*unused*/, bool /*unused1*/, const char* /*unused2*/)
{}

void retro_unload_game()
{}

auto retro_get_region() -> unsigned
{ return RETRO_REGION_NTSC; }
