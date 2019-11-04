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


#include <algorithm>
#include <string>
#include <libgen.h>

#include <stdlib.h>
#include <stdarg.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <unistd.h>
#endif

#include <cmath>

#include <libco.h>
#include "libretro.h"
#include "libretro_dosbox.h"
#include "file/file_path.h"
#include "libretro_core_options.h"

#include "setup.h"
#include "dosbox.h"
#include "mapper.h"
#include "render.h"
#include "mixer.h"
#include "control.h"
#include "pic.h"
#include "joystick.h"
#include "ints/int10.h"
#include "dos/drives.h"
#include "programs.h"

#ifdef ANDROID
#include "nonlibc.h"
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

#ifdef HAVE_LIBNX
#include <switch.h>
extern "C" Jit dynarec_jit;
#endif

#ifdef _WIN32
char slash = '\\';
#else
char slash = '/';
#endif

cothread_t mainThread;
cothread_t emuThread;

bool autofire;
bool dosbox_initialiazed = false;
bool midi_enable = false;

unsigned deadzone;
float mouse_speed_factor_x = 1.0;
float mouse_speed_factor_y = 1.0;

Bit32u MIXER_RETRO_GetFrequency();
void MIXER_CallBack(void * userdata, uint8_t *stream, int len);

extern Config* control;
extern Bit8u herc_pal;
MachineType machine = MCH_VGA;
SVGACards svgaCard = SVGA_None;

enum { CDROM_USE_SDL, CDROM_USE_ASPI, CDROM_USE_IOCTL_DIO, CDROM_USE_IOCTL_DX, CDROM_USE_IOCTL_MCI };

/* input variables */
bool gamepad[16]; /* true means gamepad, false means joystick */
bool connected[16];
bool emulated_mouse;

/* core option variables */
static bool use_core_options;
static bool adv_core_options;
CoreTimingMethod core_timing = CORE_TIMING_UNSYNCED;

/* directories */
std::string retro_save_directory;
std::string retro_system_directory;
std::string retro_content_directory;
std::string retro_library_name = "DOSBox-SVN";

/* libretro variables */
retro_video_refresh_t video_cb;
retro_audio_sample_batch_t audio_batch_cb;
retro_input_poll_t poll_cb;
retro_input_state_t input_cb;
retro_environment_t environ_cb;
retro_log_printf_t log_cb;
extern struct retro_midi_interface *retro_midi_interface;

/* DOSBox state */
static std::string loadPath;
static std::string gamePath;
static std::string configPath;
bool dosbox_exit;
static bool frontend_exit;
static bool is_restarting = false;

/* video variables */
extern Bit8u RDOSGFXbuffer[1024*768*4];
extern Bit8u RDOSGFXhaveFrame[sizeof(RDOSGFXbuffer)];
extern Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
extern unsigned RDOSGFXcolorMode;
unsigned currentWidth, currentHeight;
#define DEFAULT_FPS 60.0f
float currentFPS = DEFAULT_FPS;

/* audio variables */
static uint8_t audioData[829 * 4]; // 49716hz max
static uint32_t samplesPerFrame = 735;
static struct retro_midi_interface midi_interface;
bool disney_init;

/* callbacks */
void retro_set_video_refresh(retro_video_refresh_t cb) { video_cb = cb; }
void retro_set_audio_sample(retro_audio_sample_t cb) { }
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { audio_batch_cb = cb; }
void retro_set_input_poll(retro_input_poll_t cb) { poll_cb = cb; }
void retro_set_input_state(retro_input_state_t cb) { input_cb = cb; }

/* disk-control variables */
char disk_array[16][PATH_MAX_LENGTH];
unsigned disk_index = 0;
unsigned disk_count = 0;
bool disk_tray_ejected;
char disk_load_image[PATH_MAX_LENGTH];
bool mount_overlay = true;

/* helper functions */
static char last_written_character = 0;

void write_out_buffer(const char * format,...) {
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

void write_out (const char * format,...)
{
    write_out_buffer("\n");
    write_out_buffer(format);
}

bool mount_overlay_filesystem(char drive, const char* path)
{
    Bit16u sizes[4];
    Bit8u mediaid;
    Bit8u o_error = 0;
    std::string str_size;
    str_size="512,32,32765,16000";
    mediaid=0xF8;
    Bit8u bit8size=(Bit8u) sizes[1];

    DOS_Drive * overlay;

    localDrive* ldp = dynamic_cast<localDrive*>(Drives[drive-'A']);
    cdromDrive* cdp = dynamic_cast<cdromDrive*>(Drives[drive-'A']);

    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] mounting %s in %c as overlay\n", path, drive);

    struct stat path_stat;

    if (stat(path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode))
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] save directory already exists %s\n", path);
    }
    else
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] creating save directory %s\n", path);
#if (WIN32)
        if (mkdir(path) == -1)
#else
        if (mkdir(path, 0700) == -1)
#endif
        {
            if (log_cb)
                log_cb(RETRO_LOG_INFO, "[dosbox] error creating save directory %s\n", path);
            return false;
        }
    }
    if (!Drives[drive-'A'])
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] base drive %c is not mounted\n", drive);
        write_out("No basedrive mounted yet!");
        return false;
    }

    if (!ldp || cdp)
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] base drive %c is not compatible\n", drive);
        return false;
    }
    std::string base = ldp->getBasedir();
    overlay = new Overlay_Drive(base.c_str(), path, sizes[0], bit8size, sizes[2], sizes[3], mediaid, o_error);


    if (overlay)
    {
        if (o_error)
        {
            if (o_error == 1)
            {
                if (log_cb)
                    log_cb(RETRO_LOG_INFO, "[dosbox] can't mix absolute and relative paths");
            }
            else if (o_error == 2)
            {
                if (log_cb)
                    log_cb(RETRO_LOG_INFO, "[dosbox] overlay can't be in the same underlying file system");
            }
            else
            {
                if (log_cb)
                    log_cb(RETRO_LOG_INFO, "[dosbox] something went wrong");
            }
            delete overlay;
            return false;
        }
        delete Drives[drive-'A'];
        Drives[drive-'A'] = 0;
    }
    else
    {
        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] overlay construction failed");
        return false;
    }
    Drives[drive - 'A'] = overlay;
    mem_writeb(Real2Phys(dos.tables.mediaid) + (drive-'A') * 9, overlay->GetMediaByte());
    std::string label;
    label = drive; label += "_OVERLAY";
    overlay->dirCache.SetLabel(label.c_str(), false, false);
    return true;
}

bool mount_disk_image(const char *path, bool silent)
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

        int error = -1;
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
        RealPt save_dta = dos.dta();
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
    else
        return false;
    return false;
}

bool unmount_disk_image(char *path)
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
    Drives[drive - 'A'] = 0;
    DriveManager::CycleDisks(drive - 'A', true);

    return true;
}

bool compare_dosbox_variable(std::string section_string, std::string var_string, std::string val_string)
{
    bool ret = false;
    Section* section = control->GetSection(section_string);
    Section_prop *secprop = static_cast <Section_prop*>(section);
    if (secprop)
        ret =section->GetPropValue(var_string) == val_string;

    return ret;
}

bool update_dosbox_variable(std::string section_string, std::string var_string, std::string val_string)
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
        section->ExecuteDestroy(false);
        std::string inputline = var_string + "=" + val_string;
        ret = section->HandleInputline(inputline.c_str());
        section->ExecuteInit(false);
    }
    log_cb(RETRO_LOG_INFO, "[dosbox] variable %s::%s updated\n", section_string.c_str(), var_string.c_str(), val_string.c_str());
    return ret;
}

/* libretro core implementation */
static unsigned disk_get_num_images()
{
    return disk_count;
}

static bool disk_get_eject_state()
{
    return disk_tray_ejected;
}

static unsigned disk_get_image_index()
{
    return disk_index;
}

static bool disk_set_eject_state(bool ejected)
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
    else
    {
        if(mount_disk_image(disk_array[disk_get_image_index()], true))
            return true;
        else
            return false;
    }

    return false;
}

static bool disk_set_image_index(unsigned index)
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

static bool disk_add_image_index()
{
    disk_count++;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] disk count %u\n", disk_count);
    return true;
}

static bool disk_replace_image_index(unsigned index, const struct retro_game_info *info)
{
    if (index < disk_get_num_images())
        snprintf(disk_array[index], sizeof(char) * PATH_MAX_LENGTH, "%s", info->path);
    else
        disk_count--;
    return false;
}

static struct retro_disk_control_callback disk_interface = {
    disk_set_eject_state,
    disk_get_eject_state,
    disk_get_image_index,
    disk_set_image_index,
    disk_get_num_images,
    disk_replace_image_index,
    disk_add_image_index,
};

static void leave_thread(Bitu)
{
    MIXER_CallBack(0, audioData, samplesPerFrame * 4);
    co_switch(mainThread);

    if (core_timing != CORE_TIMING_SYNCED)
        /* Schedule the next frontend interrupt */
        PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
}

static void update_gfx_mode(bool change_fps)
{
    const float old_fps = currentFPS;
    struct retro_system_av_info new_av_info;
    bool cb_error = false;
    retro_get_system_av_info(&new_av_info);

    new_av_info.geometry.base_width = RDOSGFXwidth;
    new_av_info.geometry.base_height = RDOSGFXheight;

    if (change_fps)
    {
        const float new_fps = (core_timing == CORE_TIMING_SYNCED || core_timing == CORE_TIMING_MATCH_FPS)
                              ? render.src.fps
                              : DEFAULT_FPS;

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
        log_cb(RETRO_LOG_INFO,"[dosbox] resolution changed %dx%d @ %.3fHz => %dx%d @ %.3fHz\n",
               currentWidth, currentHeight, old_fps, RDOSGFXwidth, RDOSGFXheight, currentFPS);

    currentWidth = RDOSGFXwidth;
    currentHeight = RDOSGFXheight;
}

void check_variables()
{
    struct retro_variable var = {0};
    static unsigned cycles, cycles_fine, cycles_limit;
    static unsigned cycles_multiplier, cycles_multiplier_fine;
    static bool update_cycles = false;
    char   cycles_mode[12];
    struct retro_core_option_display option_display;

    var.key = "dosbox_svn_use_options";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        if (strcmp(var.value, "true") == 0)
            use_core_options = true;
        else
            use_core_options = false;
    }

    var.key = "dosbox_svn_core_timing";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        CoreTimingMethod old_timing = core_timing;

        if (strcmp(var.value, "synced") == 0)
            core_timing = CORE_TIMING_SYNCED;
        else if (strcmp(var.value, "match_fps") == 0)
            core_timing = CORE_TIMING_MATCH_FPS;
        else
            core_timing = CORE_TIMING_UNSYNCED;

        if (dosbox_initialiazed)
        {
            if (old_timing == CORE_TIMING_SYNCED && core_timing != CORE_TIMING_SYNCED)
            {
                DOSBOX_UnlockSpeed(false);
                PIC_AddEvent(leave_thread, 1000.0f / currentFPS);
            }
            if (old_timing != CORE_TIMING_SYNCED && core_timing == CORE_TIMING_SYNCED)
                DOSBOX_UnlockSpeed(true);
            if (old_timing != CORE_TIMING_UNSYNCED && core_timing == CORE_TIMING_UNSYNCED)
                update_gfx_mode(true);
        }
    }

    if (!use_core_options)
        return;


    var.key = "dosbox_svn_adv_options";
    var.value = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
    {
        struct retro_core_option_display option_display;
        adv_core_options = option_display.visible = strcmp(var.value, "true") == 0 ? true : false;

		option_display.key     = "dosbox_svn_sblaster_base";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_sblaster_irq";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_sblaster_dma";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_sblaster_hdma";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_sblaster_opl_mode";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_sblaster_opl_emu";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_tandy";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

		option_display.key     = "dosbox_svn_disney";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
    }

    if (!dosbox_initialiazed)
    {
        var.key = "dosbox_svn_memory_size";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("dosbox", "memsize", var.value);

        var.key = "dosbox_svn_machine_type";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            svgaCard = SVGA_None;
            machine = MCH_VGA;
            int10.vesa_nolfb = false;
            int10.vesa_oldvbe = false;
            if (strcmp(var.value, "hercules") == 0)
                machine = MCH_HERC;
            else if (strcmp(var.value, "cga") == 0)
                machine = MCH_CGA;
            else if (strcmp(var.value, "pcjr") == 0)
                machine = MCH_PCJR;
            else if (strcmp(var.value, "tandy") == 0)
                machine = MCH_TANDY;
            else if (strcmp(var.value, "ega") == 0)
                machine = MCH_EGA;
            else if (strcmp(var.value, "svga_s3") == 0)
            {
                machine = MCH_VGA;
                svgaCard = SVGA_S3Trio;
            }
            else if (strcmp(var.value, "svga_et4000") == 0)
            {
                machine = MCH_VGA;
                svgaCard = SVGA_TsengET4K;
            }
            else if (strcmp(var.value, "svga_et3000") == 0)
            {
                machine = MCH_VGA;
                svgaCard = SVGA_TsengET3K;
            }
            else if (strcmp(var.value, "svga_paradise") == 0)
            {
                machine = MCH_VGA;
                svgaCard = SVGA_ParadisePVGA1A;
            }
            else if (strcmp(var.value, "vesa_nolfb") == 0)
            {
                machine = MCH_VGA;
                svgaCard = SVGA_S3Trio;
                int10.vesa_nolfb = true;
            }
            else if (strcmp(var.value, "vesa_nolfb") == 0)
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
            update_dosbox_variable("dosbox", "machine", var.value);
        }

        var.key = "dosbox_svn_save_overlay";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !dosbox_initialiazed)
        {
            if (strcmp(var.value, "true") == 0)
                mount_overlay = true;
            else
                mount_overlay = false;
        }

    }
    else
    {
        /* hercules core options */
        bool hercules = machine == MCH_HERC;
        option_display.visible = adv_core_options && hercules;

        option_display.key     = "dosbox_svn_machine_hercules_palette";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        var.key = "dosbox_svn_machine_hercules_palette";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && machine == MCH_HERC)
        {
            herc_pal = atoi(var.value);
            Herc_Palette();
            VGA_DAC_CombineColor(1,7);
        }

        /* cga core options */
        bool cga = machine == MCH_CGA;
        option_display.visible = adv_core_options && cga;

        option_display.key     = "dosbox_svn_machine_cga_composite_mode";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        option_display.key     = "dosbox_svn_machine_cga_model";
        environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

        var.key = "dosbox_svn_machine_cga_composite_mode";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && machine == MCH_CGA)
            CGA_Composite_Mode(atoi(var.value));

        var.key = "dosbox_svn_machine_cga_model";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && machine == MCH_CGA)
            CGA_Model(var.value == 0 ? true : false);

        var.key = "dosbox_svn_sblaster_type";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            update_dosbox_variable("sblaster", "sbtype", var.value);

            /* hide blaster options if sbtype is none */
            bool blaster = strcmp(var.value, "none") != 0 ? true : false;
            option_display.visible = adv_core_options && blaster;

            option_display.key     = "dosbox_svn_sblaster_base";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_sblaster_irq";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_sblaster_dma";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_sblaster_hdma";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_sblaster_opl_mode";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_sblaster_opl_emu";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
        }

        var.key = "dosbox_svn_sblaster_base";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "sbbase", var.value);

        var.key = "dosbox_svn_sblaster_irq";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "irq", var.value);

        var.key = "dosbox_svn_sblaster_dma";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "dma", var.value);

        var.key = "dosbox_svn_sblaster_hdma";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "hdma", var.value);

        var.key = "dosbox_svn_sblaster_opl_mode";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "oplmode", var.value);

        var.key = "dosbox_svn_sblaster_opl_emu";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("sblaster", "oplemu", var.value);


        var.key = "dosbox_svn_emulated_mouse";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            bool prev = emulated_mouse;
            if (strcmp(var.value, "true") == 0)
                emulated_mouse = true;
            else
                emulated_mouse = false;

            if (prev != emulated_mouse)
                MAPPER_Init();
        }

        var.key = "dosbox_svn_emulated_mouse_deadzone";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            unsigned prev = deadzone;
            deadzone = atoi(var.value);

            if (prev != deadzone)
                MAPPER_Init();
        }

        var.key = "dosbox_svn_mouse_speed_factor_x";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            mouse_speed_factor_x = atof(var.value);

        var.key = "dosbox_svn_mouse_speed_factor_y";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            mouse_speed_factor_y = atof(var.value);

        var.key = "dosbox_svn_cpu_cycles_mode";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            snprintf(cycles_mode, sizeof(cycles_mode), "%s", var.value);
            update_cycles = true;

            option_display.visible = strcmp(var.value, "fixed") == 0 ? true : false;

            option_display.key     = "dosbox_svn_cpu_cycles";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_cpu_cycles_multiplier";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_cpu_cycles_fine";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.key     = "dosbox_svn_cpu_cycles_multiplier_fine";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);

            option_display.visible = strcmp(var.value, "max") == 0 ? true : false;

            option_display.key     = "dosbox_svn_cpu_cycles_limit";
            environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
        }

        var.key = "dosbox_svn_cpu_cycles_limit";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            cycles_limit = atoi(var.value);
            update_cycles = true;
        }

        var.key = "dosbox_svn_cpu_cycles";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            cycles = atoi(var.value);
            update_cycles = true;
        }

        var.key = "dosbox_svn_cpu_cycles_multiplier";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            cycles_multiplier = atoi(var.value);
            update_cycles = true;
        }

        var.key = "dosbox_svn_cpu_cycles_fine";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            cycles_fine = atoi(var.value);
            update_cycles = true;
        }

        var.key = "dosbox_svn_cpu_cycles_multiplier_fine";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
        {
            cycles_multiplier_fine = atoi(var.value);
            update_cycles = true;
        }

        var.key = "dosbox_svn_cpu_type";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("cpu", "cputype", var.value);

        var.key = "dosbox_svn_cpu_core";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("cpu", "core", var.value);

        var.key = "dosbox_svn_scaler";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("render", "scaler", var.value);

        var.key = "dosbox_svn_joystick_timed";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("joystick", "timed", var.value);

        if (update_cycles)
        {
            if (!strcmp(cycles_mode, "fixed"))
            {
                char s[32];
                snprintf(s, sizeof(s), "%d", cycles * cycles_multiplier + cycles_fine * cycles_multiplier_fine);
                update_dosbox_variable("cpu", "cycles", s);
            }
            else if (!strcmp(cycles_mode, "max"))
            {
                char s[32];
                snprintf(s, sizeof(s), "%s %d%%", cycles_mode, cycles_limit);
                update_dosbox_variable("cpu", "cycles", s);
            }
            else
                update_dosbox_variable("cpu", "cycles", cycles_mode);

            update_cycles = false;
        }

        var.key = "dosbox_svn_pcspeaker";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("speaker", "pcspeaker", var.value);

        var.key = "dosbox_svn_midi";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            midi_enable = true;

    #if defined(C_IPX)
        var.key = "dosbox_svn_ipx";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
            update_dosbox_variable("ipx", "ipx", var.value);
    #endif

        var.key = "dosbox_svn_tandy";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !dosbox_initialiazed)
            update_dosbox_variable("speaker", "tandy", var.value);

        var.key = "dosbox_svn_disney";
        var.value = NULL;
        if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value && !dosbox_initialiazed)
        {
            update_dosbox_variable("speaker", "disney", var.value);
            if (!strcmp(var.value,"on"))
                disney_init = true;
            else
                disney_init = false;
        }
    }
}

static void start_dosbox(void)
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
    co_switch(mainThread);

    dosbox_initialiazed = true;
    check_variables();

    if (midi_enable)
    {
        if(environ_cb(RETRO_ENVIRONMENT_GET_MIDI_INTERFACE, &midi_interface))
            retro_midi_interface = &midi_interface;
        else
            retro_midi_interface = NULL;

        if (log_cb)
            log_cb(RETRO_LOG_INFO, "[dosbox] MIDI interface %s.\n",
                retro_midi_interface ? "initialized" : "unavailable\n");
    }

    if (core_timing == CORE_TIMING_SYNCED)
        /* In synced mode, frontend takes care of timing, not dosbox */
        DOSBOX_UnlockSpeed(true);
    else
        /* When not synced, schedule the first frontend interrupt */
        PIC_AddEvent(leave_thread, 1000.0f / currentFPS);

    try
    {
        control->StartUp();
    }
    catch(int)
    {
        if (log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] frontend asked to exit\n");
        return;
    }

    if (log_cb)
        log_cb(RETRO_LOG_WARN, "[dosbox] core asked to exit\n");

    dosbox_exit = true;
}

static void wrap_dosbox()
{
    start_dosbox();

    if (emuThread && mainThread)
        co_switch(mainThread);

    /* Dead emulator */
    while(true)
    {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "[dosbox] running a dead DOSBox instance\n");
        co_switch(mainThread);
    }
}

void init_threads(void)
{
    if(!mainThread)
    {
        mainThread = co_active();
    }
    if(!emuThread)
    {
#ifdef __GENODE__
        emuThread = co_create((1<<18)*sizeof(void*), wrap_dosbox);
#else
        emuThread = co_create(65536*sizeof(void*)*16, wrap_dosbox);
#endif
    }
    else
    {
        if (log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] init called more than once \n");
    }
}

void restart_program(std::vector<std::string> & parameters)
{

    if (log_cb)
        log_cb(RETRO_LOG_WARN, "[dosbox] program restart not supported\n");
    return;

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

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_environment(retro_environment_t cb)
{
    environ_cb = cb;

    bool allow_no_game = true;

    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &allow_no_game);
    cb(RETRO_ENVIRONMENT_SET_DISK_CONTROL_INTERFACE, &disk_interface);

    libretro_set_core_options(cb);

    static const struct retro_controller_description ports_default[] =
    {
        { "Keyboard + Mouse",    RETRO_DEVICE_KEYBOARD },
        { "Gamepad",             RETRO_DEVICE_JOYPAD },
        { "Joystick",            RETRO_DEVICE_JOYSTICK },
        { "Disconnected",        RETRO_DEVICE_NONE },
        { 0 },
    };
    static const struct retro_controller_description ports_keyboard[] =
    {
        { "Keyboard + Mouse", RETRO_DEVICE_KEYBOARD },
        { "Disconnected",     RETRO_DEVICE_NONE },
        { 0 },
    };

    static const struct retro_controller_info ports[] = {
        { ports_default,  4 },
        { ports_default,  4 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        { ports_keyboard, 2 },
        { 0 },
    };
    environ_cb(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, (void*)ports);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
    connected[port] = false;
    gamepad[port]    = false;
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

void retro_get_system_info(struct retro_system_info *info)
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

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    info->geometry.base_width = 320;
    info->geometry.base_height = 200;
    info->geometry.max_width = 1024;
    info->geometry.max_height = 768;
    info->geometry.aspect_ratio = (float)4/3;
    info->timing.fps = currentFPS;
    info->timing.sample_rate = (double)MIXER_RETRO_GetFrequency();
}

void retro_init (void)
{
    /* Initialize logger interface */
    struct retro_log_callback log;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;
    else
        log_cb = NULL;

    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] logger interface initialized\n");

    RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_XRGB8888;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &RDOSGFXcolorMode);

    init_threads();

    const char *system_dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &system_dir) && system_dir)
        retro_system_directory = system_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] SYSTEM_DIRECTORY: %s\n", retro_system_directory.c_str());

    const char *save_dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &save_dir) && save_dir)
        retro_save_directory = save_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] SAVE_DIRECTORY: %s\n", retro_save_directory.c_str());

    const char *content_dir = NULL;
    if (environ_cb(RETRO_ENVIRONMENT_GET_CONTENT_DIRECTORY, &content_dir) && content_dir)
        retro_content_directory = content_dir;
    if (log_cb)
        log_cb(RETRO_LOG_INFO, "[dosbox] CONTENT_DIRECTORY: %s\n", retro_content_directory.c_str());
}

void retro_deinit(void)
{
    frontend_exit = !dosbox_exit;

    if(emuThread)
    {
        /* If the frontend wants to exit we need to let the emulator
           run to finish its job. */
        if(frontend_exit)
            co_switch(emuThread);

        co_delete(emuThread);
        emuThread = 0;
    }

#ifdef HAVE_LIBNX
	jitClose(&dynarec_jit);
#endif
}

bool retro_load_game(const struct retro_game_info *game)
{
    if(emuThread)
    {
        if(game)
        {
            /* Copy the game path */
            loadPath = normalize_path(game->path);
            const size_t lastDot = loadPath.find_last_of('.');
            char tmp[PATH_MAX_LENGTH];
            snprintf(tmp, sizeof(tmp), "%s", game->path);
            gamePath = std::string(tmp);

            /* Find any config file to load */
            if(std::string::npos != lastDot)
            {
                std::string extension = loadPath.substr(lastDot + 1);
                std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

                if(extension == "conf")
                {
                    configPath = loadPath;
                    loadPath.clear();
                }
                else if(extension == "iso" || extension == "cue")
                {
                    configPath = normalize_path(retro_save_directory + slash +  retro_library_name + ".conf");
                    if(log_cb)
                        log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
                    disk_add_image_index();
                    snprintf(disk_load_image, sizeof(disk_load_image), "%s", loadPath.c_str());
                    loadPath.clear();
                }
                else if(configPath.empty())
                {
                    configPath = normalize_path(retro_save_directory + slash +  retro_library_name + ".conf");
                    if(log_cb)
                        log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
                }
            }
        }
        else
        {
            configPath = normalize_path(retro_save_directory + slash +  retro_library_name + ".conf");
            if(log_cb)
                log_cb(RETRO_LOG_INFO, "[dosbox] loading default configuration %s\n", configPath.c_str());
        }

        co_switch(emuThread);
        samplesPerFrame = MIXER_RETRO_GetFrequency() / currentFPS;
        return true;
    }
    else
    {
        if(log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] load game called without emulator thread\n");
        return false;
    }
}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    return false;
}

void retro_run (void)
{
    /* TO-DO: Add a core option for this */
    if (dosbox_exit && emuThread) {
        co_delete(emuThread);
        emuThread = NULL;
        environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, 0);
        return;
    }

    if (core_timing != CORE_TIMING_SYNCED) {
        bool fast_forward = false;
        environ_cb(RETRO_ENVIRONMENT_GET_FASTFORWARDING, &fast_forward);
        DOSBOX_UnlockSpeed(fast_forward);
    }

    if (disk_load_image[0]!='\0')
    {
        mount_disk_image(disk_load_image, false);
        snprintf(disk_array[0], sizeof(char) * PATH_MAX_LENGTH, "%s", disk_load_image);
        disk_load_image[0] = '\0';
    }

    /* Dynamic resolution switching */
    if (RDOSGFXwidth != currentWidth || RDOSGFXheight != currentHeight ||
        (core_timing != CORE_TIMING_UNSYNCED && fabs(currentFPS - render.src.fps) > 0.05f && render.src.fps != 0))
    {
        update_gfx_mode(core_timing != CORE_TIMING_UNSYNCED);
    }

    bool updated = false;
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
        check_variables();

    if(emuThread)
    {
        /* Once C is mounted, mount the overlay */
        if (Drives['C' - 'A'] && mount_overlay)
        {
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
        co_switch(emuThread);

        if (core_timing == CORE_TIMING_SYNCED)
            MIXER_CallBack(0, audioData, samplesPerFrame * 4);
        else
            /* Upload video */
            video_cb(RDOSGFXhaveFrame, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
        /* Upload audio */
        audio_batch_cb((int16_t*)audioData, samplesPerFrame);
    }
    else
    {
        if (log_cb)
            log_cb(RETRO_LOG_WARN, "[dosbox] run called without emulator thread\n");
    }
    if (midi_enable && retro_midi_interface && retro_midi_interface->output_enabled())
        retro_midi_interface->flush();
    samplesPerFrame = MIXER_RETRO_GetFrequency() / currentFPS;
}

void retro_reset (void)
{
    restart_program(control->startup_params);
}

/* Stubs */
void *retro_get_memory_data(unsigned type) { return 0; }
size_t retro_get_memory_size(unsigned type) { return 0; }
size_t retro_serialize_size (void) { return 0; }
bool retro_serialize(void *data, size_t size) { return false; }
bool retro_unserialize(const void * data, size_t size) { return false; }
void retro_cheat_reset(void) { }
void retro_cheat_set(unsigned unused, bool unused1, const char* unused2) { }
void retro_unload_game (void) { }
unsigned retro_get_region (void) { return RETRO_REGION_NTSC; }
