// This is copyrighted software. More information is at the end of this file.
#include "CoreOptions.h"
#include "libretro_dosbox.h"

// clang-format off

#define CYCLES_COARSE_MULTIPLIERS \
    100, \
    1000, \
    10000, \
    100000,

#define CYCLES_FINE_MULTIPLIERS \
    1, \
    10, \
    100, \
    1000, \
    10000,

#define CYCLES_VALUES 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

#define MOUSE_SPEED_FACTORS \
    { \
        "0.10", "0.11", "0.12", "0.13", "0.14", "0.15", "0.16", "0.17", "0.18", "0.19", "0.20", \
        "0.21", "0.22", "0.23", "0.24", "0.25", "0.26", "0.27", "0.28", "0.29", "0.30", "0.31", \
        "0.32", "0.33", "0.34", "0.35", "0.36", "0.37", "0.38", "0.39", "0.40", "0.43", "0.45", \
        "0.48", "0.50", "0.55", "0.60", "0.65", "0.70", "0.75", "0.80", "0.85", "0.90", "0.95", \
        "1.00", "1.10", "1.17", "1.25", "1.38", "1.50", "1.63", "1.75", "2.00", "2.25", "2.50", \
        "2.75", "3.00", "3.25", "3.50", "3.75", "4.00", "4.25", "4.50", "4.75", "5.00", \
    }

namespace retro {

#if 1
CoreOptions core_options {
    "dosbox_core_",

    CoreOptionDefinition {
        "use_options",
        "Enable core options (restart)",
        "Disabling core options can be useful if you prefer to use dosbox .conf files to set "
            "configuration settings. Note that you can still use core options together with "
            ".conf files, for example if your .conf files only contain an [autoexec] section, "
            "or dosbox settings not yet available as core options.",
        {
            true,
            false,
        },
        true
    },
    CoreOptionDefinition {
        "adv_options",
        "Show all options",
        "Show all options, including those that usually do not require changing.",
        {
            true,
            false,
        },
        false
    },
    CoreOptionCategory {
        "timing",
        "Timing",
        "Timing and synchronization.",

        CoreOptionDefinition {
            "core_timing",
            "Frame timing mode",
            "External mode is the recommended setting. It enables the frontend to drive frame "
                "pacing. It has no input lag and allows frontend features like DRC and Frame Delay "
                "to work correctly. Internal mode runs out of sync with the frontend. This allows "
                "for 60FPS output when running 70FPS games, but input lag and stutter/judder are "
                "increased. If you have a high refresh rate display (70Hz or better,) then use "
                "external mode. It also works great with VRR (g-sync/freesync.) Internal mode is "
                "mostly only useful for 60Hz displays in order to get rid of tearing in 70FPS "
                "games.",
            {
                "external",
                { "internal", "internal (fixed 60FPS)" },
            },
            "external"
        },
        CoreOptionDefinition {
            "thread_sync",
            "Thread synchronization method",
            "\"Wait\" is the recommended method and should work well on most systems. If for some "
                "reason it doesn't and you're seeing stutter, setting this to \"spin\" might help "
                "(or it might make it worse.) However, \"spin\" will also result in 100% usage on "
                "one of your CPU cores. This is \"idle load\" and doesn't increase CPU "
                "temperatures by much, but it will prevent the CPU from clocking down which on "
                "laptops will affect battery life. ",
            {
                "wait",
                "spin",
            },
            "wait"
        },
    },
    CoreOptionCategory {
        "file_and_disk",
        "Storage",
        "File and disk options.",

        CoreOptionDefinition {
            "mount_c_as",
            "Mount drive C as",
            "When directly loading a DOS executable rather than a .conf file, the C drive can be "
                "mounted to be either the executable's directory, or its parent directory. For "
                "example, when loading DUKE3D.EXE that is located in a directory called DUKE3D, "
                "setting this option to \"content\" will result in DOS finding the file in "
                "C:\\DUKE3D.EXE. If this option is set to \"parent\", the file will be found in "
                "C:\\DUKE3D\\DUKE3D.EXE instead.",
            {
                { "content", "content directory" },
                { "parent", "parent directory of content" },
            },
            "content"
        },
        CoreOptionDefinition {
            "default_mount_freesize",
            "Free space for default-mounted drive C",
            "This is the \"-freesize\" value to use for drive C when loading a DOS executable "
                "instead of a .conf file that contains its own MOUNT command.",
            {
                { 256, "256MB" },
                { 384, "384MB" },
                { 512, "512MB" },
                { 768, "768MB" },
                { 1024, "1GB" },
                { 1280, "1.25GB" },
                { 1536, "1.5GB" },
                { 1792, "1.75GB" },
            },
            1024
        },
        CoreOptionDefinition {
            "save_overlay",
            "Enable overlay file system (restart)",
            "Enable overlay file system to redirect filesystem changes to the save directory. "
                "Disable if you have problems starting some games.",
            {
                true,
                false,
            },
            false
        },
    },
    CoreOptionCategory {
        "video_emulation",
        "Video card",
        "Configuration of the emulated video card.",

        CoreOptionDefinition {
            "machine_type",
            "Emulated machine (restart)",
            "The type of video hardware DOSBox will emulate.",
            {
                { "hercules", "Hercules" },
                { "cga", "CGA" },
                { "tandy", "Tandy" },
                { "pcjr", "PCjr" },
                { "ega", "EGA" },
                { "vgaonly", "VGA" },
                { "svga_s3", "SVGA (S3 Trio64)" },
                { "vesa_nolfb", "SVGA (S3 Trio64 no-line buffer hack)" },
                { "vesa_oldvbe", "SVGA (S3 Trio64 VESA 1.3)" },
                { "svga_et3000", "SVGA (Tseng Labs ET3000)" },
                { "svga_et4000", "SVGA (Tseng Labs ET4000)" },
                { "svga_paradise", "SVGA (Paradise PVGA1A)" },
            },
            "svga_s3"
        },
        CoreOptionDefinition {
            "machine_hercules_palette",
            "Hercules color mode",
            "The color scheme for hercules emulation.",
            {
                { 0, "black & white" },
                { 1, "black & amber" },
                { 2, "black & green" },
            },
            0
        },
        CoreOptionDefinition {
            "machine_cga_composite_mode",
            "CGA composite mode toggle",
            "Enable or disable CGA composite mode.",
            {
                { 0, "auto" },
                { 1, "true" },
                { 2, "false" },
            },
            0
        },
        CoreOptionDefinition {
            "machine_cga_model",
            "CGA model",
            "The type of CGA model in the emulated system.",
            {
                { 0, "late" },
                { 1, "early" },
            },
            0
        },
        #ifdef WITH_VOODOO
        CoreOptionDefinition {
            "voodoo",
            "3dfx Voodoo emulation (restart)",
            "This emulates the actual 3dfx hardware. This is not a glide emulator and as a result "
                "a glide wrapper is neither needed nor supported.\n"
                "Only slow (VERY slow), software-based emulation is supported at the moment. It is "
                "probably not possible to get playable speeds in most games.",
            {
            // Not implemented yet.
            #if 0
                "opengl",
            #endif
                "software",
                { false, "none" },
            },
            false
        },
        CoreOptionDefinition {
            "voodoo_memory_size",
            "Voodoo memory size (restart)",
            "The amount of memory that the emulated Voodoo card has. 4MB is the standard memory "
                "configuration for the original Voodoo. 12MB is a non-standard configuration.",
            {
                { "standard", "4MB" },
                { "max", "12MB" },
            },
            "standard"
        },
        #endif
    },
    CoreOptionCategory {
        "specs",
        "Specs",
        "CPU and RAM specifications of the emulated DOS PC.",

        CoreOptionDefinition {
            "memory_size",
            "Memory size (restart)",
            "The amount of memory that the emulated machine has. This value is best left at its "
                "default to avoid problems with some games, though few games might require a higher "
                "value.",
            {
                { 4, "4MB" },
                { 8, "8MB" },
                { 16, "16MB" },
                { 24, "24MB" },
                { 32, "32MB" },
                { 48, "48MB" },
                { 64, "64MB" },
            },
            16
        },
        CoreOptionDefinition {
            "cpu_core",
            "CPU core",
            "CPU core used for emulation. "
        #if defined(C_DYNREC) || defined(C_DYNAMIC_X86)
                "When set to \"auto\", the \"normal\" interpreter core will be used for real mode "
                "games, while the faster \"dynamic\" recompiler core will be used for protected "
                "mode games. The \"simple\" interpreter core is optimized for old real mode games.",
            {
                "auto",
            #if defined(C_DYNREC)
                { "dynamic", "dynamic (generic recompiler)" },
            #else
                { "dynamic", "dynamic (x86 recompiler)" },
            #endif
                "normal",
                "simple",
            },
            "auto"
        #else
            "\"Simple\" is optimized for old real-mode games. (There are no dynamic recompiler "
                "cores available on this platform.)",
            {
                "normal",
                "simple",
            },
            "normal"
        #endif
        },
        CoreOptionDefinition {
            "cpu_type",
            "CPU type",
            "Emulated CPU type. \"Auto\" is the fastest choice.",
            {
                "auto",
                "386",
                { "386_slow", "386 (slow)" },
                { "386_prefetch", "386 (prefetch queue emulation)" },
                "486",
                { "486_slow", "486 (slow)" },
                { "pentium_slow", "pentium (slow)" },
            },
            "auto"
        },
        CoreOptionDefinition {
            "cpu_cycles_mode",
            "CPU cycles mode",
            "Method to determine the amount of emulated CPU cycles per millisecond. \"Fixed\" mode "
                "emulates the amount of cycles you have set. \"Max\" mode will emulate as many "
                "cycles as possible, depending on the limits you have set. You can configure a "
                "maximum CPU load percentage as well as a cycle amount as limits. \"Auto\" will "
                "emulate the fixed cycle amount set in the \"real mode\" cycles options when "
                "running real mode games, while for protected mode games it will switch to \"max\" "
                "mode. \"Fixed\" mode in combination with an appropriate cycle amount is the most "
                "compatible setting, as \"auto\" and \"max\" have issues on many systems.",
            {
                "auto",
                "fixed",
                "max",
            },
            "fixed"
        },
        CoreOptionDefinition {
            "cpu_cycles_multiplier_realmode",
            "Real mode coarse CPU cycles multiplier",
            "Multiplier for coarse CPU cycles tuning when running real mode games in \"auto\" "
                "cycles mode.",
            { CYCLES_COARSE_MULTIPLIERS },
            1000
        },
        CoreOptionDefinition {
            "cpu_cycles_realmode",
            "Real mode coarse CPU cycles value",
            "Value for coarse CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_VALUES },
            3
        },
        CoreOptionDefinition {
            "cpu_cycles_multiplier_fine_realmode",
            "Real mode fine CPU cycles multiplier",
            "Multiplier for fine CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_FINE_MULTIPLIERS },
            100
        },
        CoreOptionDefinition {
            "cpu_cycles_fine_realmode",
            "Real mode fine CPU cycles value",
            "Value for fine CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_VALUES },
            0
        },
        CoreOptionDefinition {
            "cpu_cycles_limit",
            "Max CPU cycles limit",
            "Limit the maximum amount of CPU cycles used when using \"max\" mode.",
            {
                "none",
                "10%",
                "20%",
                "30%",
                "40%",
                "50%",
                "60%",
                "70%",
                "80%",
                "90%",
                "100%",
                "105%",
            },
            "100%"
        },
        CoreOptionDefinition {
            "cpu_cycles_multiplier",
            "Coarse CPU cycles multiplier",
            "Multiplier for coarse CPU cycles tuning.",
            { CYCLES_COARSE_MULTIPLIERS },
            10000
        },
        CoreOptionDefinition {
            "cpu_cycles",
            "Coarse CPU cycles value",
            "Value for coarse CPU cycles tuning.",
            { CYCLES_VALUES },
            1
        },
        CoreOptionDefinition {
            "cpu_cycles_multiplier_fine",
            "Fine CPU cycles multiplier",
            "Multiplier for fine CPU cycles tuning.",
            { CYCLES_FINE_MULTIPLIERS },
            1000
        },
        CoreOptionDefinition {
            "cpu_cycles_fine",
            "Fine CPU cycles value",
            "Value for fine CPU cycles tuning.",
            { CYCLES_VALUES },
            0
        },
    },
    CoreOptionCategory {
        "scaling",
        "Scaling",
        "Image scaling options.",

        CoreOptionDefinition {
            "aspect_correction",
            "Aspect ratio correction",
            "When enabled, the aspect ratio will match that of a CRT monitor. This is required for "
                "non-square pixel resolutions to look as intended. Disable this if you want "
                "unscaled square pixel aspect ratios (at the cost of a squashed or stretched "
                "image), or if the result looks clearly wrong (games that use 640x350 for example "
                "will look stretched with this enabled.)",
            {
                true,
                false,
            },
            true
        },
        CoreOptionDefinition {
            "scaler",
            "DOSBox scaler",
            "Built-in, CPU-based DOSBox scalers. These are provided here only as a last resort. "
                "You should generally set this to \"none\" and instead use the scaling options and "
                "shaders that are provided by your frontend.",
            {
                "normal2x",
                "normal3x",
                "advmame2x",
                "advmame3x",
                "advinterp2x",
                "advinterp3x",
                "hq2x",
                "hq3x",
                "2xsai",
                "super2xsai",
                "supereagle",
                "tv2x",
                "tv3x",
                "rgb2x",
                "rgb3x",
                "scan2x",
                "scan3x",
                "none",
            },
            "none"
        },
    },
    CoreOptionCategory {
        "input",
        "Input",
        "Emulated joystick and mouse.",

        CoreOptionDefinition {
            "joystick_force_2axis",
            "Force 2-axis/2-button",
            "Normally, when only one port is assigned a joystick or gamepad, 4 axes and 4 buttons "
                "are emulated on that port. Some (usually older) games however do not work "
                "correctly without a classic 2-axis/2-button joystick.",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "joystick_timed",
            "Enable joystick timed intervals",
            "Enable timed intervals for joystick axes. Experiment with this option if your "
                "joystick drifts.",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "emulated_mouse",
            "Enable gamepad emulated mouse",
            "Enable mouse emulation via the right stick on your gamepad.",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "emulated_mouse_deadzone",
            "Gamepad emulated mouse deadzone",
            "Deadzone of the gamepad emulated mouse. Experiment with this value if the mouse "
                "cursor drifts.",
            {
                { 0, "0%" },
                { 5, "5%" },
                { 10, "10%" },
                { 15, "15%" },
                { 20, "20%" },
                { 25, "25%" },
                { 30, "30%" },
            },
            30
        },
        CoreOptionDefinition {
            "mouse_speed_factor_x",
            "Horizontal mouse sensitivity",
            "Experiment with this value if the mouse is too fast when moving left/right.",
            MOUSE_SPEED_FACTORS,
            "1.00"
        },
        CoreOptionDefinition {
            "mouse_speed_factor_y",
            "Vertical mouse sensitivity",
            "Experiment with this value if the mouse is too fast when moving up/down.",
            MOUSE_SPEED_FACTORS,
            "1.00"
        },
    },
    CoreOptionCategory {
        "vkbd",
        "Virtual keyboard",
        "On-screen virtual keyboard.",

        CoreOptionDefinition {
            "vkbd_theme",
            "Color theme",
            {
                { "light", "Light (shadow)" },
                { "light_outline", "Light (outline)" },
                { "dark", "Dark (shadow)" },
                { "dark_outline", "Dark (outline)" },
            },
            "light",
        },
        CoreOptionDefinition {
            "vkbd_transparency",
            "Transparency",
            {
                "0%",
                "25%",
                "50%",
                "75%",
                "100%",
            },
            "25%",
        },
    },
    CoreOptionCategory {
        "sound_card",
        "Sound",
        "Emulated audio device parameters.",

        CoreOptionDefinition {
            "sblaster_type",
            "SoundBlaster type",
            "Type of emulated SoundBlaster card.",
            {
                { "sb1", "SoundBlaster 1.0" },
                { "sb2", "SoundBlaster 2.0" },
                { "sbpro1", "SoundBlaster Pro" },
                { "sbpro2", "SoundBlaster Pro 2" },
                { "sb16", "SoundBlaster 16" },
                { "gb", "GameBlaster" },
                { "none", "none" },
            },
            "sb16"
        },
        CoreOptionDefinition {
            "sblaster_base",
            "SoundBlaster Base Address",
            "The I/O address for the emulated SoundBlaster card.",
            {
                "220",
                "240",
                "260",
                "280",
                "2a0",
                "2c0",
                "2e0",
                "300",
            },
            "220"
        },
        CoreOptionDefinition {
            "sblaster_irq",
            "SoundBlaster IRQ Number",
            "The IRQ number for the emulated SoundBlaster card.",
            {
                3,
                5,
                7,
                9,
                10,
                11,
                12,
            },
            7
        },
        CoreOptionDefinition {
            "sblaster_dma",
            "SoundBlaster DMA Number",
            "The DMA number for the emulated SoundBlaster card.",
            {
                0,
                1,
                3,
                5,
                6,
                7,
            },
            1
        },
        CoreOptionDefinition {
            "sblaster_hdma",
            "SoundBlaster High DMA Number",
            "The High DMA number for the emulated SoundBlaster card.",
            {
                0,
                1,
                3,
                5,
                6,
                7,
            },
            5
        },
        CoreOptionDefinition {
            "sblaster_opl_mode",
            "SoundBlaster OPL mode",
            "The SoundBlaster emulated OPL mode. All modes are Adlib compatible except cms.",
            {
                { "auto", "auto (select based on the SoundBlaster type)" },
                { "cms", "CMS (Creative Music System / GameBlaster)" },
                { "opl2", "OPL-2 (AdLib / OPL-2 / Yamaha 3812)" },
                { "dualopl2", "Dual OPL-2 (used by SoundBlaster Pro 1.0 for stereo sound)" },
                { "opl3", "OPL-3 (AdLib / OPL-3 / Yamaha YMF262)" },
                { "opl3gold", "OPL-3 Gold (AdLib Gold / OPL-3 / Yamaha YMF262)" },
                "none",
            },
            "auto"
        },
        CoreOptionDefinition {
            "sblaster_opl_emu",
            "SoundBlaster OPL provider",
            "\"Nuked OPL3\" is a cycle-accurate OPL3 (YMF262) emulator. It offers the best "
                "quality, but is quite demanding on the CPU. \"Compat\" is the next best option. "
                "It is less accurate, but also less demanding.",
            {
                { "nuked", "Nuked OPL3" },
                "compat",
                "mame",
                "fast",
            },
            "compat"
        },
        CoreOptionDefinition {
            "gus",
            "Gravis Ultrasound support",
            "Enables Gravis Ultrasound emulation. The ULTRADIR directory is not configurable. It "
                "is always set to C:\\ULTRASND.",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "gusbase",
            "Ultrasound IO address",
            "The IO base address for the emulated Gravis Ultrasound card.",
            {
                "220",
                "240",
                "260",
                "280",
                "2a0",
                "2c0",
                "2e0",
                "300",
            },
            "240"
        },
        CoreOptionDefinition {
            "gusirq",
            "Ultrasound IRQ",
            "The IRQ number for the emulated Gravis Ultrasound card.",
            {
                3,
                5,
                7,
                9,
                10,
                11,
                12,
            },
            5
        },
        CoreOptionDefinition {
            "gusdma",
            "Ultrasound DMA",
            "The DMA channel for the emulated Gravis Ultrasound card.",
            {
                0,
                1,
                3,
                5,
                6,
                7,
            },
            3
        },
        CoreOptionDefinition {
            "pcspeaker",
            "Enable PC speaker",
            "Enable PC speaker emulation.",
            {
                true,
                false,
            },
            true
        },
        CoreOptionDefinition {
            "tandy",
            "Enable Tandy Sound System",
            "Enable Tandy Sound System Emulation. Auto only works if machine is set to tandy.",
            {
                "auto",
                { "on", "true" },
                { "off", "false" },
            },
            "off"
        },
        CoreOptionDefinition {
            "disney",
            "Enable Disney Sound Source",
            "Enable Disney Sound Source Emulation.",
            {
                { "on", "true" },
                { "off", "false" },
            },
            "off"
        },
        CoreOptionDefinition {
            "blocksize",
            "DOSBox mixer block size (restart)",
            "Larger blocks might help sound stuttering but sound will also be more lagged.",
            {
                256,
                512,
                1024,
                2048,
                4096,
                8192,
            },
            1024
        },
        CoreOptionDefinition {
            "prebuffer",
            "DOSBox mixer pre-buffer size (restart)",
            "How many milliseconds of data to keep on top of the blocksize.",
            {
                0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24, 25, 26, 28,
                30, 32, 36, 40, 44, 48, 56, 64, 72, 80, 88, 96, 100
            },
            25
        },
    },
    CoreOptionCategory {
        "midi",
        "MIDI",
        "MIDI emulation and output.",

        CoreOptionDefinition {
            "mpu_type",
            "MPU-401 type",
            "Type of MPU-401 MIDI interface to emulate. \"Intelligent\" mode is the best choice.",
            {
                "intelligent",
                { "uart", "UART" },
                "none",
            },
            "intelligent"
        },
        CoreOptionDefinition {
            "midi_driver",
            "MIDI driver",
            "The MT-32 emulation driver uses Munt and needs the correct ROMs in the frontend's "
                "system directory.\n"
            #ifdef WITH_BASSMIDI
                "For BASSMIDI, you need to download the BASS and BASSMIDI libraries for your OS "
                "from https://www.un4seen.com and place them in the frontend's system directory.\n"
            #endif
                "The libretro driver forwards MIDI to the frontend, in which case you need to "
                "configure MIDI output there.",
            {
            #ifdef HAVE_ALSA
                { "alsa", "ALSA" },
            #endif
            #ifdef __WIN32__
                { "win32", "Windows MIDI" },
            #endif
            #ifdef WITH_BASSMIDI
                { "bassmidi", "BASSMIDI" },
            #endif
            #ifdef WITH_FLUIDSYNTH
                { "fluidsynth", "FluidSynth" },
            #endif
                { "mt32", "MT-32 emulator" },
                "libretro",
                "none",
            },
            "none"
        },
        #ifdef HAVE_ALSA
        CoreOptionDefinition {
            "midi_port",
            "ALSA MIDI port",
            "ALSA port to send MIDI to."
            // No values. We detect and set MIDI ports at runtime.
        },
        #endif
        #ifdef __WIN32__
        CoreOptionDefinition {
            "midi_port",
            "Windows MIDI port",
            "Windows port to send MIDI to."
            // No values. We detect and set MIDI ports at runtime.
        },
        #endif
        #ifdef WITH_BASSMIDI
        CoreOptionDefinition {
            "bassmidi.soundfont",
            "BASSMIDI soundfont",
            "Soundfonts are looked for in the \"soundfonts\" directory inside the frontend's "
                "system directory. Supported formats are SF2 and SFZ.",
            // No values. We scan for soundfonts at runtime.
        },
        CoreOptionDefinition {
            "bassmidi.sfvolume",
            "BASSMIDI soundfont volume",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
                "1.1",
                "1.2",
                "1.3",
                "1.4",
                "1.5",
                "1.6",
                "1.7",
                "1.8",
                "1.9",
                "2.0",
                "2.5",
                "3.0",
                "3.5",
                "4.0",
                "4.5",
                "5.0",
                "6.0",
                "7.0",
                "8.0",
                "9.0",
                "10.0",
            },
            "0.6",
        },
        CoreOptionDefinition {
            "bassmidi.voices",
            "BASSMIDI voice count",
            "Maximum number of samples that can play together. This is not the same thing as the "
                "maximum number of notes; multiple samples may be played for a single note. ",
            {
                20,
                30,
                40,
                50,
                60,
                70,
                80,
                90,
                100,
                120,
                140,
                160,
                180,
                200,
                250,
                300,
                350,
                400,
                450,
                500,
                600,
                700,
                800,
                900,
                1000,
            },
            100
        },
        #endif
        #ifdef WITH_FLUIDSYNTH
        CoreOptionDefinition {
            "fluid.soundfont",
            "FluidSynth soundfont",
            "Soundfonts are looked for in the \"soundfonts\" directory inside the frontend's "
                "system directory. Supported formats are SF2, SF3, DLS and GIG. SF2 and SF3 are "
                "the recommended formats.",
            // No values. We scan for soundfonts at runtime.
        },
        CoreOptionDefinition {
            "fluid.samplerate",
            "FluidSynth sample rate",
            "The sample rate of the audio generated by the synthesizer.",
            {
                { 8000, "8kHz" },
                { 11025, "11.025kHz" },
                { 16000, "16kHz" },
                { 22050, "22.05kHz" },
                { 32000, "32kHz" },
                { 44100, "44.1kHz" },
                { 48000, "48kHz" },
                { 96000, "96kHz" },
            },
            44100
        },
        CoreOptionDefinition {
            "fluid.gain",
            "FluidSynth volume gain",
            "The volume gain is applied to the final output of the synthesizer. Usually this needs "
                "to be rather low (0.2-0.5) for most soundfonts to avoid audio clipping and "
                "distortion. ",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
                "1.1",
                "1.2",
                "1.3",
                "1.4",
                "1.5",
                "1.6",
                "1.7",
                "1.8",
                "1.9",
                "2.0",
                "2.5",
                "3.0",
                "3.5",
                "4.0",
                "4.5",
                "5.0",
                "6.0",
                "7.0",
                "8.0",
                "9.0",
                "10.0",
            },
            "0.4",
        },
        CoreOptionDefinition {
            "fluid.polyphony",
            "FluidSynth polyphony",
            "The polyphony defines how many voices can be played in parallel. Higher values are "
                "more CPU intensive.",
            {
                1,
                2,
                4,
                8,
                16,
                32,
                64,
                128,
                256,
                384,
                512,
                768,
                1024,
                1536,
                2048,
                3072,
                4096,
            },
            256
        },
        CoreOptionDefinition {
            "fluid.cores",
            "FluidSynth CPU cores",
            "Sets the number of synthesis CPU cores. If set to a value greater than 1, then "
                "additional synthesis threads will be created to take advantage of a multi CPU or "
                "CPU core system.",
            {
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                16,
                20,
                24,
                32
            },
            1
        },
        CoreOptionDefinition {
            "fluid.reverb",
            "FluidSynth enable reverb",
            {
                true,
                false,
            },
            true
        },
        CoreOptionDefinition {
            "fluid.reverb.roomsize",
            "FluidSynth reverb room size",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
            },
            "0.2"
        },
        CoreOptionDefinition {
            "fluid.reverb.damping",
            "FluidSynth reverb damping",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
            },
            "0.0"
        },
        CoreOptionDefinition {
            "fluid.reverb.width",
            "FluidSynth reverb width",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.1",
                "1.2",
                "1.3",
                "1.4",
                "1.5",
                "1.6",
                "1.7",
                "1.8",
                "1.9",
                "2.0",
                "2.5",
                "3.0",
                "3.5",
                "4.0",
                "4.5",
                "5.0",
                "6.0",
                "7.0",
                "8.0",
                "9.0",
                "10.0",
                "12.0",
                "14.0",
                "16.0",
                "18.0",
                "20.0",
                "25.0",
                "30.0",
                "35.0",
                "40.0",
                "45.0",
                "50.0",
                "60.0",
                "70.0",
                "80.0",
                "90.0",
                "100.0",
            },
            "0.5"
        },
        CoreOptionDefinition {
            "fluid.reverb.level",
            "FluidSynth reverb level",
            {
                "0.0",
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
            },
            "0.9"
        },
        CoreOptionDefinition {
            "fluid.chorus",
            "FluidSynth enable chorus",
            {
                true,
                false,
            },
            true
        },
        CoreOptionDefinition {
            "fluid.chorus.number",
            "FluidSynth chorus voices",
            {
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9,
                10,
                11,
                12,
                14,
                16,
                18,
                20,
                24,
                28,
                32,
                56,
                64,
                96,
            },
            3
        },
        CoreOptionDefinition {
            "fluid.chorus.level",
            "FluidSynth chorus level",
            {
                "0.0",
                "0.2",
                "0.4",
                "0.6",
                "0.8",
                "1.0",
                "1.2",
                "1.4",
                "1.6",
                "1.8",
                "2.0",
                "2.2",
                "2.4",
                "2.6",
                "2.8",
                "3.0",
                "3.5",
                "4.0",
                "4.5",
                "5.0",
                "5.5",
                "6.0",
                "6.5",
                "7.0",
                "7.5",
                "8.0",
                "8.5",
                "9.0",
                "9.5",
                "10.0",
            },
            "2.0"
        },
        CoreOptionDefinition {
            "fluid.chorus.speed",
            "FluidSynth chorus speed",
            {
                "0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                "0.6",
                "0.7",
                "0.8",
                "0.9",
                "1.0",
                "1.1",
                "1.2",
                "1.3",
                "1.4",
                "1.5",
                "1.6",
                "1.7",
                "1.8",
                "1.9",
                "2.0",
                "2.1",
                "2.2",
                "2.3",
                "2.4",
                "2.5",
                "2.6",
                "2.7",
                "2.8",
                "2.9",
                "3.0",
                "3.1",
                "3.2",
                "3.3",
                "3.4",
                "3.5",
                "3.6",
                "3.7",
                "3.8",
                "3.9",
                "4.0",
                "4.1",
                "4.2",
                "4.3",
                "4.4",
                "4.5",
                "4.6",
                "4.7",
                "4.8",
                "4.9",
                "5.0",
            },
            "0.3"
        },
        CoreOptionDefinition {
            "fluid.chorus.depth",
            "FluidSynth chorus depth",
            {
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
                8,
                9,
                10,
                11,
                12,
                13,
                14,
                15,
                16,
                18,
                20,
                22,
                24,
                28,
                32,
                48,
                64,
                80,
                96,
                112,
                128,
                160,
                192,
                224,
                256,
            },
            8
        },
        #endif
        CoreOptionDefinition {
            "mt32.type",
            "MT-32 hardware type",
            "Type of MT-32 module to emulate. MT-32 is the older, original model. The CM-32L "
                "and LAPC-I are later models that provide some extra instruments not found on the "
                "original MT-32. Some games make use of these extra sounds and won't sound correct "
                "on the original MT-32.",
            {
                { "mt32", "MT-32" },
                { "cm32l", "CM-32L/LAPC-I" },
            },
            "cm32l"
        },
        CoreOptionDefinition {
            "mt32.reverse.stereo",
            "MT-32 reverse stereo channels",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "mt32.thread",
            "MT-32 threaded emulation",
            "Run MT-32 emulation in its own thread. Improves performance on multi-core CPUs.",
            {
                true,
                false,
            },
            false
        },
        CoreOptionDefinition {
            "mt32.chunk",
            "MT-32 threaded chunk size",
            "Minimum milliseconds of data to render at once. Increasing this value reduces "
                "rendering overhead which may improve performance but also increases audio lag.",
            {
                { 2, "2ms" },
                { 3, "3ms" },
                { 4, "4ms" },
                { 5, "5ms" },
                { 7, "7ms" },
                { 10, "10ms" },
                { 13, "13ms" },
                { 16, "16ms" },
                { 20, "20ms" },
                { 24, "24ms" },
                { 28, "28ms" },
                { 32, "32ms" },
                { 40, "40ms" },
                { 48, "48ms" },
                { 56, "56ms" },
                { 64, "64ms" },
                { 80, "80ms" },
                { 96, "96ms" },
            },
            16
        },
        CoreOptionDefinition {
            "mt32.prebuffer",
            "MT-32 threaded prebuffer size",
            "How many milliseconds of data to render ahead. Increasing this value may help to "
                "avoid underruns but also increases audio lag. Cannot be set less than or equal to "
                "the chunk size value.",
            {
                { 3, "3ms" },
                { 4, "4ms" },
                { 5, "5ms" },
                { 7, "7ms" },
                { 10, "10ms" },
                { 13, "13ms" },
                { 16, "16ms" },
                { 20, "20ms" },
                { 24, "24ms" },
                { 28, "28ms" },
                { 32, "32ms" },
                { 40, "40ms" },
                { 48, "48ms" },
                { 56, "56ms" },
                { 64, "64ms" },
                { 80, "80ms" },
                { 96, "96ms" },
                { 112, "112ms" },
                { 128, "128ms" },
                { 144, "144ms" },
                { 160, "160ms" },
                { 176, "176ms" },
                { 192, "192ms" },
            },
            32
        },
        CoreOptionDefinition {
            "mt32.partials",
            "MT-32 max partials",
            "The maximum number of partials playing simultaneously. A value of 32 matches real "
                "MT-32 hardware. Lowering this value increases performance at the cost of notes "
                "getting cut off sooner. Increasing it allows more notes to stay audible compared "
                "to real hardware at the cost of performance.",
            {
                8,
                9,
                10,
                11,
                12,
                14,
                16,
                20,
                24,
                28,
                32,
                40,
                48,
                56,
                64,
                72,
                80,
                96,
                112,
                128,
                144,
                160,
                176,
                192,
                224,
                256,
            },
            32
        },
        CoreOptionDefinition {
            "mt32.dac",
            "MT-32 DAC input emulation mode",
            "High quality: Produces samples at double the volume, without tricks. Higher quality "
                "than the real devices.\n"
                "Pure: Produces samples that exactly match the bits output from the emulated LA32. "
                "Nicer overdrive characteristics than the DAC hacks (it simply clips samples "
                "within range.) Much less likely to overdrive than any other mode. Half the volume "
                "of any of the other modes.\n"
                "Gen 1: Re-orders the LA32 output bits as in the early generation MT-32.\n"
                "Gen 2: Re-orders the LA32 output bits as in the later generations MT-32 and "
                "CM-32Ls.",
            {
                { 0, "high quality" },
                { 1, "pure" },
                { 2, "gen 1" },
                { 3, "gen 2" },
            },
            0
        },
        CoreOptionDefinition {
            "mt32.analog",
            "MT-32 analog output emulation mode",
            "Digital: Only digital path is emulated. The output samples correspond to the digital "
                "output signal appeared at the DAC entrance. Fastest mode.\n"
                "Coarse: Coarse emulation of LPF circuit. High frequencies are boosted, sample "
                "rate remains unchanged. A bit better sounding but also a bit slower.\n"
                "Accurate: Finer emulation of LPF circuit. Output signal is upsampled to 48 kHz to "
                "allow emulation of audible mirror spectra above 16 kHz, which is passed through "
                "the LPF circuit without significant attenuation. Sounding is closer to the analog "
                "output from real hardware but also slower than \"digital\" and \"coarse\".\n"
                "Oversampled: Same as \"accurate\" but the output signal is 2x oversampled, i.e. "
                "the output sample rate is 96 kHz. Even slower than all the other modes but better "
                "retains highest frequencies while further resampled in DOSBox mixer.",
            {
                { 0, "digital" },
                { 1, "coarse" },
                { 2, "accurate" },
                { 3, "oversampled" },
            },
            2
        },
        CoreOptionDefinition {
            "mt32.reverb.mode",
            "MT-32 reverb mode",
            "Reverb emulation mode. \"Auto\" will automatically adjust reverb parameters to match "
                "the loaded control ROM version.",
            {
                { "auto" },
                { 0, "room" },
                { 1, "hall" },
                { 2, "plate" },
                { 3, "tap delay" },
            },
            "auto"
        },
        CoreOptionDefinition {
            "mt32.reverb.time",
            "MT-32 reverb decay time",
            {
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
            },
            5
        },
        CoreOptionDefinition {
            "mt32.reverb.level",
            "MT-32 reverb level",
            {
                0,
                1,
                2,
                3,
                4,
                5,
                6,
                7,
            },
            3
        },
        CoreOptionDefinition {
            "mt32.rate",
            "MT-32 sample rate",
            {
                { 8000, "8kHz" },
                { 11025, "11.025kHz" },
                { 16000, "16kHz" },
                { 22050, "22.05kHz" },
                { 32000, "32kHz" },
                { 44100, "44.1kHz" },
                { 48000, "48kHz" },
                { 49716, "49.716kHz" },
            },
            44100
        },
        CoreOptionDefinition {
            "mt32.src.quality",
            "MT-32 resampling quality",
            {
                { 0, "fastest" },
                { 1, "fast" },
                { 2, "good" },
                { 3, "best" },
            },
            2
        },
        CoreOptionDefinition {
            "mt32.niceampramp",
            "MT-32 nice amp ramp",
            "Improves amplitude ramp for sustaining instruments. Quick changes of volume or "
                "expression on a MIDI channel may result in amp jumps on real hardware. Enabling "
                "this option prevents this from happening. Disabling this options preserves "
                "emulation accuracy.",
            {
                true,
                false,
            },
            true
        },
    },
    CoreOptionDefinition {
        "ipx",
        "Enable IPX networking",
        "Enable IPX over UDP tunneling.",
        {
            true,
            false,
        },
        false
    },
    CoreOptionCategory {
        "logging",
        "Logging",
        "Event logging.",

        CoreOptionDefinition {
            "log_method",
            "Output method",
            "Where to send log output. \"Frontend\" will send it to the frontend, while "
                "\"stdout/stderr\" will print it to standard output or standard error (warnings "
                "and errors go to stderr, debug and informational messages to stdout.)",
            {
                "frontend",
                "stdout/stderr",
            },
            "frontend",
        },
        CoreOptionDefinition {
            "log_level",
            "Verbosity level",
            {
                "debug",
                "info",
                "warnings",
                "errors",
            },
            "warnings",
        },
    },
};
#endif

// clang-format on

} // namespace retro

/*

Copyright (C) 2015-2018 Andrés Suárez
Copyright (C) 2019 Nikos Chantziaras.

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
