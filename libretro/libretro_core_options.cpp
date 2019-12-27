// This is copyrighted software. More information is at the end of this file.
#include "CoreOptions.h"
#include "libretro_dosbox.h"

#define MOUSE_SPEED_FACTORS \
    { \
        { "0.10" }, { "0.11" }, { "0.12" }, { "0.13" }, { "0.14" }, { "0.15" }, { "0.16" }, \
        { "0.17" }, { "0.18" }, { "0.19" }, { "0.20" }, { "0.21" }, { "0.22" }, { "0.23" }, \
        { "0.24" }, { "0.25" }, { "0.26" }, { "0.27" }, { "0.28" }, { "0.29" }, { "0.30" }, \
        { "0.31" }, { "0.32" }, { "0.33" }, { "0.34" }, { "0.35" }, { "0.36" }, { "0.37" }, \
        { "0.38" }, { "0.39" }, { "0.40" }, { "0.43" }, { "0.45" }, { "0.48" }, { "0.50" }, \
        { "0.55" }, { "0.60" }, { "0.65" }, { "0.70" }, { "0.75" }, { "0.80" }, { "0.85" }, \
        { "0.90" }, { "0.95" }, { "1.00" }, { "1.10" }, { "1.17" }, { "1.25" }, { "1.38" }, \
        { "1.50" }, { "1.63" }, { "1.75" }, { "2.00" }, { "2.25" }, { "2.50" }, { "2.75" }, \
        { "3.00" }, { "3.25" }, { "3.50" }, { "3.75" }, { "4.00" }, { "4.25" }, { "4.50" }, \
        { "4.75" }, { "5.00" }, \
    }

retro::CoreOptions retro::core_options {
    "dosbox_svn_",
    {
        {
            "use_options",
            "Core: Enable options",
            "Enable options. Disable in-case of using pre-generated configuration files (restart).",
            {
                { true },
                { false },
            },
            true
        },
        {
            "adv_options",
            "Core: Enable advanced options",
            "Enable advanced options that are not required for normal operation.",
            {
                { true },
                { false },
            },
            false
        },
        {
            "save_overlay",
            "Core: Enable overlay file system (restart)",
            "Enable overlay file system to redirect filesystem changes to the save directory. "
                "Disable if you have problems starting some games.",
            {
                { true },
                { false },
            },
            false
        },
        {
            "core_timing",
            "Core: Timing mode",
            "The internal modes work on an internal scheduler. DOSBox will render frames at its "
                "own pace which results in additional input lag and periodic stutter, but cycles "
                "modes \"auto\" and \"max\" should work as intended.\n\n"
                "External mode works based on the frontend's scheduler. It has no input lag and "
                "does not produce stutter, but requires a fixed cycle rate.",
            {
                { "internal_fixed", "internal (fixed 60fps)" },
                { "internal_variable", "internal (variable fps)" },
                { "external", "external (variable fps)" },
            },
            "internal_fixed"
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
            "The amount of memory that the emulated machine has.",
            {
                { 4 },
                { 8 },
                { 16 },
                { 24 },
                { 32 },
                { 48 },
                { 64 },
            },
            32
        },
#if defined(C_DYNREC) || defined(C_DYNAMIC_X86)
        {
            "cpu_core",
            "System: CPU core",
#if defined(C_DYNREC)
            "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core "
                "DYNREC available.",
#else
            "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core "
                "DYNAMIC_X86 available.",
#endif
            {
                { "auto", "auto (real-mode games use normal, protected-mode games use dynamic if "
                          "available)" },
#if defined(C_DYNREC)
                { "dynamic", "dynamic (dynarec using dynrec implementation)" },
#else
                { "dynamic", "dynamic (dynarec using dynamic_x86 implementation)" },
#endif
                { "normal", "normal (interpreter)" },
                { "simple", "simple (interpreter optimized for old real-mode games)" },
            },
            "auto"
        },
#else
        {
            "cpu_core",
            "System: CPU core",
            "CPU core used for emulation. Theare are no dynamic cores available on this platform.",
            {
                { "normal", "normal (interpreter)" },
                { "simple", "simple (interpreter optimized for old real-mode games)" },
            },
            "normal"
        },
#endif
        {
            "cpu_type",
            "System: CPU type",
            "Emulated CPU type. Auto is the fastest choice.",
            {
                { "auto", "auto (fastest choice)" },
                { "386", "386" },
                { "386_slow", "386 (slow)" },
                { "386_prefetch", "386 (prefetch queue emulation)" },
                { "486", "486" },
                { "486_slow", "486 (slow)" },
                { "pentium_slow", "pentium (slow)" },
            },
            "auto"
        },
        {
            "cpu_cycles_mode",
            "System: CPU cycles mode",
            "Method to determine the amount of CPU cycles that DOSBox tries to emulate per "
                "milisecond. Use auto unless you have performance problems. A value that is too "
                "high for your system may cause slowdown.",
            {
                { "auto", "auto (sets 3000 cycles in real mode games, max in protected mode "
                          "games)" },
                { "fixed", "fixed (manually configure emulated CPU speed" },
                { "max", "max (sets cycles to default value of the host CPU)" },
            },
            "auto"
        },
        {
            "cpu_cycles_multiplier",
            "System: Coarse CPU cycles multiplier",
            "Multiplier for coarse CPU cycles tuning.",
            {
                { 100 },
                { 1000 },
                { 10000 },
                { 100000 },
            },
            1000
        },
        {
            "cpu_cycles",
            "System: Coarse CPU cycles value",
            "Value for coarse CPU cycles tuning.",
            {
                { 0 },
                { 1 },
                { 2 },
                { 3 },
                { 4 },
                { 5 },
                { 6 },
                { 7 },
                { 8 },
                { 9 },
            },
            1
        },
        {
            "cpu_cycles_multiplier_fine",
            "System: Fine CPU cycles multiplier",
            "Multiplier for fine CPU cycles tuning.",
            {
                { 1 },
                { 10 },
                { 100 },
                { 1000 },
                { 10000 },
            },
            100
        },
        {
            "cpu_cycles_fine",
            "System: Fine CPU cycles value",
            "Value for fine CPU cycles tuning.",
            {
                { 0 },
                { 1 },
                { 2 },
                { 3 },
                { 4 },
                { 5 },
                { 6 },
                { 7 },
                { 8 },
                { 9 },
            },
            0
        },
        {
            "cpu_cycles_limit",
            "System: Max CPU cycles limit",
            "Limit the maximum amount of CPU cycles used.",
            {
                { 10, "10%" },
                { 20, "20%" },
                { 30, "30%" },
                { 40, "40%" },
                { 50, "50%" },
                { 60, "60%" },
                { 70, "70%" },
                { 80, "80%" },
                { 90, "90%" },
                { 100, "100%" },
                { 105, "105%" },
            },
            100
        },
        {
            "aspect_correction",
            "Video: Aspect ratio correction.",
            "When enabled, the core's aspect ratio is set to what a CRT monitor would display. "
                "This is required for all non 4:3 VGA resolutions to look as intended. When "
                "disabled, the core's aspect ratio is set to match the current VGA resolution's "
                "width to height ratio, providing square pixels but resulting in a stretched or "
                "squashed image.",
            {
                { true },
                { false },
            },
            true
        },
        {
            "scaler",
            "Video: Scaler",
            "Scaler used to scale or improve image quality.",
            {
                { "none" },
                { "normal2x" },
                { "normal3x" },
                { "advmame2x" },
                { "advmame3x" },
                { "advinterp2x" },
                { "advinterp3x" },
                { "hq2x" },
                { "hq3x" },
                { "2xsai" },
                { "super2xsai" },
                { "supereagle" },
                { "tv2x" },
                { "tv3x" },
                { "rgb2x" },
                { "rgb3x" },
                { "scan2x" },
                { "scan3x" },
            },
            "none"
        },
        {
            "joystick_timed",
            "Input: Enable joystick timed intervals",
            "Enable timed intervals for joystick axes. Experiment with this option if your "
                "joystick drifts.",
            {
                { false },
                { true },
            },
            false
        },
        {
            "emulated_mouse",
            "Input: Enable gamepad emulated mouse",
            "Enable mouse emulation via the right stick on your gamepad.",
            {
                { false },
                { true },
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
                { "220" },
                { "240" },
                { "260" },
                { "280" },
                { "2a0" },
                { "2c0" },
                { "2e0" },
                { "300" },
            },
            "220"
        },
        {
            "sblaster_irq",
            "Sound: SoundBlaster IRQ Number",
            "The IRQ number for the emulated SoundBlaster card.",
            {
                { 3 },
                { 5 },
                { 7 },
                { 9 },
                { 10 },
                { 11 },
                { 12 },
            },
            7
        },
        {
            "sblaster_dma",
            "Sound: SoundBlaster DMA Number",
            "The DMA number for the emulated SoundBlaster card.",
            {
                { 0 },
                { 1 },
                { 3 },
                { 5 },
                { 6 },
                { 7 },
            },
            1
        },
        {
            "sblaster_hdma",
            "Sound: SoundBlaster High DMA Number",
            "The High DMA number for the emulated SoundBlaster card.",
            {
                { 0 },
                { 1 },
                { 3 },
                { 5 },
                { 6 },
                { 7 },
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
                { "none" },
            },
            "auto"
        },
        {
            "sblaster_opl_emu",
            "Sound: SoundBlaster OPL provider",
            "Provider for the OPL emulation. Compat might provide the best quality.",
            {
                { "default" },
                { "compat" },
                { "fast" },
                { "mame" },
            },
            "default"
        },
        {
            "gus",
            "Sound: Gravis Ultrasound support",
            "Enables Gravis Ultrasound emulation. Thee ULTRADIR directory is not configurable. It "
                "is always set to C:\\ULTRASND and is not configurable via options.",
            {
                { false },
                { true },
            },
            false
        },
        {
            "gusrate",
            "Sound: Ultrasound sample rate",
            "Gravis Ultrasound emulation sample rate.",
            {
                { 8000 },
                { 11025 },
                { 16000 },
                { 22050 },
                { 32000 },
                { 44100 },
                { 48000 },
                { 49716 },
            },
            44100
        },
        {
            "gusbase",
            "Sound: Ultrasound IO address",
            "The IO base address for the emulated Gravis Ultrasound card.",
            {
                { "220" },
                { "240" },
                { "260" },
                { "280" },
                { "2a0" },
                { "2c0" },
                { "2e0" },
                { "300" },
            },
            "240"
        },
        {
            "gusirq",
            "Sound: Ultrasound IRQ",
            "The IRQ number for the emulated Gravis Ultrasound card.",
            {
                { 3 },
                { 5 },
                { 7 },
                { 9 },
                { 10 },
                { 11 },
                { 12 },
            },
            5
        },
        {
            "gusdma",
            "Sound: Ultrasound DMA",
            "The DMA channel for the emulated Gravis Ultrasound card.",
            {
                { 0 },
                { 1 },
                { 3 },
                { 5 },
                { 6 },
                { 7 },
            },
            3
        },
        {
            "midi",
            "Sound: Enable libretro MIDI passthrough",
            "Enable libretro MIDI passthrough.",
            {
                { false },
                { true },
            },
            false
        },
        {
            "pcspeaker",
            "Sound: Enable PC speaker",
            "Enable PC speaker emulation.",
            {
                { false },
                { true },
            },
            false
        },
        {
            "tandy",
            "Sound: Enable Tandy Sound System",
            "Enable Tandy Sound System Emulation. Auto only works if machine is set to tandy.",
            {
                { "auto" },
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
                { false },
                { true },
            },
            false
        },
    }
};

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
