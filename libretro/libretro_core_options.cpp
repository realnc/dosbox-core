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

retro::CoreOptions retro::core_options {
    "dosbox_core_",
    {
        {
            "use_options",
            "Core: Enable core options (restart)",
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
        {
            "adv_options",
            "Core: Show all options",
            "Show all options, including those that usually do not require changing.",
            {
                true,
                false,
            },
            false
        },
        {
            "save_overlay",
            "Core: Enable overlay file system (restart)",
            "Enable overlay file system to redirect filesystem changes to the save directory. "
                "Disable if you have problems starting some games.",
            {
                true,
                false,
            },
            false
        },
        {
            "core_timing",
            "Core: Timing mode",
            "External mode is the recommended setting. It enables the frontend to drive frame "
                "pacing. It has no input lag and allows frontend features like DRC and Frame Delay "
                "to work correctly. Internal mode runs out of sync with the frontend. This allows "
                "for 60FPS output when running 70FPS games, but input lag and stutter/judder are "
                "increased. Use only if you absolutely need 60FPS output with no tearing in 70FPS "
                "games. ",
            {
                "external",
                { "internal", "internal (fixed 60FPS)" },
            },
            "external"
        },
        {
            "thread_sync",
            "Core: Thread synchronization method",
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
        {
            "machine_type",
            "System: Emulated machine (restart)",
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
        {
            "machine_hercules_palette",
            "System: Hercules color mode",
            "The color scheme for hercules emulation.",
            {
                { 0, "black & white" },
                { 1, "black & amber" },
                { 2, "black & green" },
            },
            0
        },
        {
            "machine_cga_composite_mode",
            "System: CGA composite mode toggle",
            "Enable or disable CGA composite mode.",
            {
                { 0, "auto" },
                { 1, "true" },
                { 2, "false" },
            },
            0
        },
        {
            "machine_cga_model",
            "System: CGA model",
            "They type of CGA model in the emulated system.",
            {
                { 0, "late" },
                { 1, "early" },
            },
            0
        },
        {
            "memory_size",
            "System: Memory size (restart)",
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
        {
            "cpu_core",
            "System: CPU core",
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
        {
            "cpu_type",
            "System: CPU type",
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
        {
            "cpu_cycles_mode",
            "System: CPU cycles mode",
            "Method to determine the amount of emulated CPU cycles per millisecond. \"Fixed\" mode "
                "emulates the amount of cycles you have set. \"Max\" mode will emulate as many "
                "cycles as possible, depending on the limits you have set. You can configure a "
                "maximum CPU load percentage as well as a cycle amount as limits. \"Auto\" will "
                "emulate the fixed cycle amount set in the \"real mode\" cycles options when "
                "running real mode games, while for protected mode games it will switch to \"max\" "
                "mode.",
            {
                "auto",
                "fixed",
                "max",
            },
            "auto"
        },
        {
            "cpu_cycles_multiplier_realmode",
            "System: Real mode coarse CPU cycles multiplier",
            "Multiplier for coarse CPU cycles tuning when running real mode games in \"auto\" "
                "cycles mode.",
            { CYCLES_COARSE_MULTIPLIERS },
            1000
        },
        {
            "cpu_cycles_realmode",
            "System: Real mode coarse CPU cycles value",
            "Value for coarse CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_VALUES },
            3
        },
        {
            "cpu_cycles_multiplier_fine_realmode",
            "System: Real mode fine CPU cycles multiplier",
            "Multiplier for fine CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_FINE_MULTIPLIERS },
            100
        },
        {
            "cpu_cycles_fine_realmode",
            "System: Real mode fine CPU cycles value",
            "Value for fine CPU cycles tuning when running real mode games in \"auto\" cycles "
                "mode.",
            { CYCLES_VALUES },
            0
        },
        {
            "cpu_cycles_limit",
            "System: Max CPU cycles limit",
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
        {
            "cpu_cycles_multiplier",
            "System: Coarse CPU cycles multiplier",
            "Multiplier for coarse CPU cycles tuning.",
            { CYCLES_COARSE_MULTIPLIERS },
            1000
        },
        {
            "cpu_cycles",
            "System: Coarse CPU cycles value",
            "Value for coarse CPU cycles tuning.",
            { CYCLES_VALUES },
            1
        },
        {
            "cpu_cycles_multiplier_fine",
            "System: Fine CPU cycles multiplier",
            "Multiplier for fine CPU cycles tuning.",
            { CYCLES_FINE_MULTIPLIERS },
            100
        },
        {
            "cpu_cycles_fine",
            "System: Fine CPU cycles value",
            "Value for fine CPU cycles tuning.",
            { CYCLES_VALUES },
            0
        },
        {
            "aspect_correction",
            "Video: Aspect ratio correction.",
            "When enabled, the aspect ratio will match that of a CRT monitor. This is required for "
                "non-square pixel VGA resolutions to look as intended. Disable this if you want "
                "unscaled square pixel aspect ratios, but this will result in a squashed or "
                "stretched image.",
            {
                true,
                false,
            },
            true
        },
        {
            "scaler",
            "Video: DOSBox scaler",
            "Built-in, CPU-based DOSBox scalers. These are provided here only as a last resort. "
                "You should generally set this to \"none\" and instead use the scaling options and "
                "shaders that are provided by your frontend.",
            {
                "none",
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
            },
            "none"
        },
        {
            "joystick_timed",
            "Input: Enable joystick timed intervals",
            "Enable timed intervals for joystick axes. Experiment with this option if your "
                "joystick drifts.",
            {
                false,
                true,
            },
            false
        },
        {
            "emulated_mouse",
            "Input: Enable gamepad emulated mouse",
            "Enable mouse emulation via the right stick on your gamepad.",
            {
                false,
                true,
            },
            false
        },
        {
            "emulated_mouse_deadzone",
            "Input: Gamepad emulated mouse deadzone",
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
        {
            "mouse_speed_factor_x",
            "Input: Horizontal mouse sensitivity.",
            "Experiment with this value if the mouse is too fast when moving left/right.",
            MOUSE_SPEED_FACTORS,
            "1.00"
        },
        {
            "mouse_speed_factor_y",
            "Input: Vertical mouse sensitivity.",
            "Experiment with this value if the mouse is too fast when moving up/down.",
            MOUSE_SPEED_FACTORS,
            "1.00"
        },
        {
            "sblaster_type",
            "Sound: SoundBlaster type",
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
        {
            "sblaster_base",
            "Sound: SoundBlaster Base Address",
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
        {
            "sblaster_irq",
            "Sound: SoundBlaster IRQ Number",
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
        {
            "sblaster_dma",
            "Sound: SoundBlaster DMA Number",
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
        {
            "sblaster_hdma",
            "Sound: SoundBlaster High DMA Number",
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
        {
            "sblaster_opl_mode",
            "Sound: SoundBlaster OPL mode",
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
        {
            "sblaster_opl_emu",
            "Sound: SoundBlaster OPL provider",
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
        {
            "gus",
            "Sound: Gravis Ultrasound support",
            "Enables Gravis Ultrasound emulation. The ULTRADIR directory is not configurable. It "
                "is always set to C:\\ULTRASND.",
            {
                false,
                true,
            },
            false
        },
        {
            "gusrate",
            "Sound: Ultrasound sample rate",
            "Gravis Ultrasound emulation sample rate.",
            {
                8000,
                11025,
                16000,
                22050,
                32000,
                44100,
                48000,
                49716,
            },
            44100
        },
        {
            "gusbase",
            "Sound: Ultrasound IO address",
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
        {
            "gusirq",
            "Sound: Ultrasound IRQ",
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
        {
            "gusdma",
            "Sound: Ultrasound DMA",
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
        {
            "mpu_type",
            "Sound: MPU-401 type",
            "Type of MPU-401 MIDI interface to emulate. \"Intelligent\" mode is the best choice.",
            {
                "intelligent",
                { "uart", "UART" },
                "none",
            },
            "intelligent"
        },
        {
            "midi_driver",
            "Sound: MIDI driver",
            "Driver to use for MIDI playback. The MT-32 emulation driver uses Munt and needs the "
                "correct ROMs in the frontend's system directory. The libretro driver forwards "
                "MIDI to the frontend, in which case you need to configure MIDI output there.",
            {
                "none",
            #ifdef HAVE_ALSA
                { "alsa", "ALSA" },
            #endif
            #ifdef __WIN32__
                { "win32", "Windows MIDI" },
            #endif
                { "mt32", "MT-32 emulator" },
                "libretro",
            },
            "none"
        },
        #ifdef HAVE_ALSA
        {
            "midi_port",
            "Sound: ALSA MIDI port",
            "ALSA port to send MIDI to."
            // No values. We detect and set MIDI ports at runtime.
        },
        #endif
        #ifdef __WIN32__
        {
            "midi_port",
            "Sound: Windows MIDI port",
            "Windows port to send MIDI to."
            // No values. We detect and set MIDI ports at runtime.
        },
        #endif
        {
            "mt32.type",
            "Sound (MT-32): Hardware type",
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
        {
            "mt32.reverse.stereo",
            "Sound (MT-32): Reverse stereo channels",
            {
                false,
                true,
            },
            false
        },
        {
            "mt32.thread",
            "Sound (MT-32): Threaded emulation",
            "Run MT-32 emulation in its own thread. Improves performance on multi-core CPUs.",
            {
                false,
                true,
            },
            false
        },
        {
            "mt32.chunk",
            "Sound (MT-32): Threaded chunk size",
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
        {
            "mt32.prebuffer",
            "Sound (MT-32): Threaded prebuffer size",
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
        {
            "mt32.partials",
            "Sound (MT-32): Max partials",
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
        {
            "mt32.dac",
            "Sound (MT-32): DAC input emulation mode",
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
        {
            "mt32.analog",
            "Sound (MT-32): Analog output emulation mode",
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
        {
            "mt32.reverb.mode",
            "Sound (MT-32): Reverb mode",
            "Reverb emulation mode. \"Auto\" will automatically adjust reverb parameters to match "
                "the loaded control ROM version.",
            {
                { 0, "room" },
                { 1, "hall" },
                { 2, "plate" },
                { 3, "tap delay" },
                { "auto" },
            },
            "auto"
        },
        {
            "mt32.reverb.time",
            "Sound (MT-32): Reverb decay time",
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
        {
            "mt32.reverb.level",
            "Sound (MT-32): Reverb level",
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
        {
            "mt32.rate",
            "Sound (MT-32): Sample rate",
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
        {
            "mt32.src.quality",
            "Sound (MT-32): Resampling quality",
            {
                { 0, "fastest" },
                { 1, "fast" },
                { 2, "good" },
                { 3, "best" },
            },
            2
        },
        {
            "mt32.niceampramp",
            "Sound (MT-32): Nice amp ramp",
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
        {
            "pcspeaker",
            "Sound: Enable PC speaker",
            "Enable PC speaker emulation.",
            {
                false,
                true,
            },
            false
        },
        {
            "tandy",
            "Sound: Enable Tandy Sound System",
            "Enable Tandy Sound System Emulation. Auto only works if machine is set to tandy.",
            {
                "auto",
                { "on", "true" },
                { "off", "false" },
            },
            "off"
        },
        {
            "disney",
            "Sound: Enable Disney Sound Source",
            "Enable Disney Sound Source Emulation.",
            {
                { "off", "false" },
                { "on", "true" },
            },
            "off"
        },
        {
            "ipx",
            "Network: Enable IPX",
            "Enable IPX over UDP tunneling.",
            {
                false,
                true,
            },
            false
        },
    }
};

// clang-format on

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
