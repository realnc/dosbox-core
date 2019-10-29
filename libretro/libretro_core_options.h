#ifndef LIBRETRO_CORE_OPTIONS_H__
#define LIBRETRO_CORE_OPTIONS_H__

#include <stdlib.h>
#include <string.h>

#include <libretro.h>
#include <retro_inline.h>

#ifndef HAVE_NO_LANGEXTRA
#include "libretro_core_options_intl.h"
#endif

/*
 ********************************
 * VERSION: 1.3
 ********************************
 *
 * - 1.3: Move translations to libretro_core_options_intl.h
 *        - libretro_core_options_intl.h includes BOM and utf-8
 *          fix for MSVC 2010-2013
 *        - Added HAVE_NO_LANGEXTRA flag to disable translations
 *          on platforms/compilers without BOM support
 * - 1.2: Use core options v1 interface when
 *        RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION is >= 1
 *        (previously required RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION == 1)
 * - 1.1: Support generation of core options v0 retro_core_option_value
 *        arrays containing options with a single value
 * - 1.0: First commit
*/

#ifdef __cplusplus
extern "C" {
#endif

/*
 ********************************
 * Core Option Definitions
 ********************************
*/

/* RETRO_LANGUAGE_ENGLISH */

/* Default language:
 * - All other languages must include the same keys and values
 * - Will be used as a fallback in the event that frontend language
 *   is not available
 * - Will be used as a fallback for any missing entries in
 *   frontend language definition */

#define MOUSE_SPEED_FACTORS \
    { \
        { "0.25", NULL }, \
        { "0.50", NULL }, \
        { "0.75", NULL }, \
        { "1.00", NULL }, \
        { "1.25", NULL }, \
        { "1.50", NULL }, \
        { "1.75", NULL }, \
        { "2.00", NULL }, \
        { "2.25", NULL }, \
        { "2.50", NULL }, \
        { "2.75", NULL }, \
        { "3.00", NULL }, \
        { "3.25", NULL }, \
        { "3.50", NULL }, \
        { "3.75", NULL }, \
        { "4.00", NULL }, \
        { "4.25", NULL }, \
        { "4.50", NULL }, \
        { "4.75", NULL }, \
        { "5.00", NULL }, \
        { NULL, NULL } \
    }

struct retro_core_option_definition option_defs_us[] = {
   {
      "dosbox_svn_use_options",
      "Core: Enable options",
      "Enable options. Disable in-case of using pre-generated configuration files (restart).",
      {
         { "true", NULL },
         { "false", NULL },
         { NULL, NULL },
      },
      "true"
   },
   {
      "dosbox_svn_adv_options",
      "Core: Enable advanced options",
      "Enable advanced options that are not required for normal operation.",
      {
         { "true", NULL },
         { "false", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_save_overlay",
      "Core: Enable overlay file system",
      "Enable overlay file system to redirect filesystem changes to the save directory. Disable if you have problems starting some games (restart).",
      {
         { "true", NULL },
         { "false", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_machine_type",
      "System: Emulated machine",
      "The type of machine that DOSBox will try to emulate (restart).",
      {
         { "hercules", "Hercules (Hercules Graphics Card)" },
         { "cga", "CGA (Color Graphics Adapter)" },
         { "tandy", "Tandy (Tandy Graphics Adapter" },
         { "pcjr", "PCjr" },
         { "ega", "EGA (Enhanced Graphics Adapter" },
         { "vgaonly", "VGA (Video Graphics Array)" },
         { "svga_s3", "SVGA (Super Video Graphics Array) (S3 Trio64)" },
         { "svga_et3000", "SVGA (Super Video Graphics Array) (Tseng Labs ET3000)" },
         { "svga_et4000", "SVGA (Super Video Graphics Array) (Tseng Labs ET4000)" },
         { "svga_paradise", "SVGA (Super Video Graphics Array) (Paradise PVGA1A)" },
         { "vesa_nolfb", "SVGA (Super Video Graphics Array) (S3 Trio64 no-line buffer hack)" },
         { "vesa_oldvbe", "SVGA (Super Video Graphics Array) (S3 Trio64 VESA 1.3)" },
         { NULL, NULL },
      },
      "svga_s3"
   },
   {
      "dosbox_svn_memory_size",
      "System: Memory size",
      "The amount of memory that the emulated machine has (restart).",
      {
         { "4", NULL },
         { "8", NULL },
         { "16", NULL },
         { "24", NULL },
         { "32", NULL },
         { "48", NULL },
         { "64", NULL },
         { NULL, NULL },
      },
      "32"
   },
#if defined(C_DYNREC) || defined(C_DYNAMIC_X86)
   {
      "dosbox_svn_cpu_core",
      "System: CPU core",
#if defined(C_DYNREC)
      "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core DYNREC available.",
#else
      "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core DYNAMIC_X86 available.",
#endif
      {
         { "auto", "auto (real-mode games use normal, protected-mode games use dynamic if available)" },
#if defined(C_DYNREC)
         { "dynamic", "dynamic (dynarec using dynrec implementation)" },
#else
         { "dynamic", "dynamic (dynarec using dynamic_x86 implementation)" },
#endif
         { "normal", "normal (interpreter)" },
         { "simple", "simple (interpreter optimized for old real-mode games)" },
         { NULL, NULL },
      },
      "auto"
   },
#else
   {
      "dosbox_svn_cpu_core",
      "System: CPU core",
      "CPU core used for emulation. Theare are no dynamic cores available on this platform.",
      {
         { "normal", "normal (interpreter)" },
         { "simple", "simple (interpreter optimized for old real-mode games)" },
         { NULL, NULL },
      },
      "auto"
   },
#endif
   {
      "dosbox_svn_cpu_type",
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
         { NULL, NULL },
      },
      "auto"
   },
   {
      "dosbox_svn_cpu_cycles_mode",
      "System: CPU cycles mode",
      "Method to determine the amount of CPU cycles that DOSBox tries to emulate per milisecond. Use auto unless you have performance problems. A value that is too high for your system may cause slowdown.",
      {
         { "auto", "auto (real-mode games use fixed cycles 3000, protected-mode games use max)" },
         { "fixed", "fixed (set emulated CPU speed to a amount of cycles" },
         { "max", "max (sets cycles to default value of the host CPU)" }, /*TO-DO: add limit*/
         { NULL, NULL },
      },
      "auto"
   },
   {
      "dosbox_svn_cpu_cycles_multiplier",
      "System: Coarse CPU cycles multiplier",
      "Multiplier for coarse CPU cycles tuning.",
      {
         { "1000", NULL },
         { "10000", NULL },
         { "100000", NULL },
         { NULL, NULL },
      },
      "1000"
   },
   {
      "dosbox_svn_cpu_cycles",
      "System: Coarse CPU cycles value",
      "Value for coarse CPU cycles tuning.",
      {
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "1"
   },
   {
      "dosbox_svn_cpu_cycles_multiplier_fine",
      "System: Fine CPU cycles multiplier",
      "Multiplier for fine CPU cycles tuning.",
      {
         { "1", NULL },
         { "10", NULL },
         { "100", NULL },
         { NULL, NULL },
      },
      "100"
   },
   {
      "dosbox_svn_cpu_cycles_fine",
      "System: Fine CPU cycles value",
      "Value for fine CPU cycles tuning.",
      {
         { "1", NULL },
         { "2", NULL },
         { "3", NULL },
         { "4", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "8", NULL },
         { "9", NULL },
         { NULL, NULL },
      },
      "1"
   },
   {
      "dosbox_svn_scaler",
      "Video: Scaler",
      "Scaler used to scale or improve image quality.",
      {
         { "none", NULL },
         { "normal2x", NULL },
         { "normal3x", NULL },
         { "advmame2x", NULL },
         { "advmame3x", NULL },
         { "advinterp2x", NULL },
         { "advinterp3x", NULL },
         { "hq2x", NULL },
         { "hq3x", NULL },
         { "2xsai", NULL },
         { "super2xsai", NULL },
         { "supereagle", NULL },
         { "tv2x", NULL },
         { "tv3x", NULL },
         { "rgb2x", NULL },
         { "rgb3x", NULL },
         { "scan2x", NULL },
         { "scan3x", NULL },
         { NULL, NULL },
      },
      "none"
   },
   {
      "dosbox_svn_use_native_refresh",
      "Video: Enable refresh rate switching",
      "Enable refresh rate switching to match running content. This is a expensive operation and may cause the screen to flicker.",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_joystick_timed",
      "Input: Enable joystick timed intervals",
      "Enable timed intervals for joystick axes. Experiment with this option if your joystick drifts (restart).",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_emulated_mouse",
      "Input: Enable gamepad emulated mouse",
      "Enable mouse emulation via the right stick on your gamepad.",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_emulated_mouse_deadzone",
      "Input: Gamepad emulated mouse deadzone",
      "Deadzone of the gamepad emulated mouse. Experiment with this value if the mouse cursor drifts.",
      {
         { "0%", NULL },
         { "5%", NULL },
         { "10%", NULL },
         { "15%", NULL },
         { "20%", NULL },
         { "25%", NULL },
         { "30%", NULL },
         { NULL, NULL },
      },
      "30%"
   },
   {
      "dosbox_svn_mouse_speed_factor_x",
      "Input: Horizontal mouse sensitivity.",
      "Experiment with this value if the mouse is too fast when moving left/right.",
      MOUSE_SPEED_FACTORS,
      "1.00"
   },
   {
      "dosbox_svn_mouse_speed_factor_y",
      "Input: Vertical mouse sensitivity.",
      "Experiment with this value if the mouse is too fast when moving up/down.",
      MOUSE_SPEED_FACTORS,
      "1.00"
   },
   {
      "dosbox_svn_sblaster_type",
      "Sound: SoundBlaster type",
      "Type of emulated SoundBlaster card (restart).",
      {
         { "sb1", "SoundBlaster 1.0" },
         { "sb2", "SoundBlaster 2.0" },
         { "sbpro1", "SoundBlaster Pro" },
         { "sbpro2", "SoundBlaster Pro 2" },
         { "sb16", "SoundBlaster 16" },
         { "gb", "GameBlaster" },
         { "none", "none" },
         { NULL, NULL },
      },
      "sb16"
   },
   {
      "dosbox_svn_sblaster_base",
      "Sound: SoundBlaster Base Address",
      "The I/O address for the emulated SoundBlaster card (restart).",
      {
         { "220", NULL },
         { "240", NULL },
         { "260", NULL },
         { "280", NULL },
         { "2a0", NULL },
         { "2c0", NULL },
         { "2e0", NULL },
         { "300", NULL },
         { NULL, NULL },
      },
      "220"
   },
   {
      "dosbox_svn_sblaster_irq",
      "Sound: SoundBlaster IRQ Number",
      "The IRQ number for the emulated SoundBlaster card (restart).",
      {
         { "3", NULL },
         { "5", NULL },
         { "7", NULL },
         { "9", NULL },
         { "10", NULL },
         { "11", NULL },
         { "12", NULL },
         { NULL, NULL },
      },
      "7"
   },
   {
      "dosbox_svn_sblaster_dma",
      "Sound: SoundBlaster DMA Number",
      "The DMA number for the emulated SoundBlaster card (restart).",
      {
         { "1", NULL },
         { "3", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "0", NULL },
         { NULL, NULL },
      },
      "1"
   },
   {
      "dosbox_svn_sblaster_hdma",
      "Sound: SoundBlaster High DMA Number",
      "The High DMA number for the emulated SoundBlaster card (restart).",
      {
         { "1", NULL },
         { "3", NULL },
         { "5", NULL },
         { "6", NULL },
         { "7", NULL },
         { "0", NULL },
         { NULL, NULL },
      },
      "7"
   },
   {
      "dosbox_svn_sblaster_opl_mode",
      "Sound: SoundBlaster OPL mode",
      "The SoundBlaster emulated OPL mode. All modes are Adlib compatible except cms (restart).",
      {
         { "auto", "auto (select based on the SoundBlaster type)" },
         { "cms", "CMS (Creative Music System / GameBlaster)" },
         { "opl2", "OPL-2 (AdLib / OPL-2 / Yamaha 3812)" },
         { "dualopl2", "Dual OPL-2 (Dual OPL-2 used by SoundBlaster Pro 1.0 for stereo sound)" },
         { "opl3", "OPL-3 (AdLib / OPL-3 / Yamaha YMF262)" },
         { "opl3gold", "OPL-3 Gold (AdLib Gold / OPL-3 / Yamaha YMF262)" },
         { "none", NULL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "dosbox_svn_sblaster_opl_emu",
      "Sound: SoundBlaster OPL provider",
      "Provider for the OPL emulation. Compat might provide the best quality (restart).",
      {
         { "default", NULL },
         { "compat", NULL },
         { "fast", NULL },
         { "mame", NULL },
         { NULL, NULL },
      },
      "default"
   },
   {
      "dosbox_svn_midi",
      "Sound: Enable libretro MIDI passthrough",
      "Enable libretro MIDI passthrough.",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_pcspeaker",
      "Sound: Enable PC speaker",
      "Enable PC speaker emulation.",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_tandy",
      "Sound: Enable Tandy Sound System",
      "Enable Tandy Sound System Emulation. Auto only works if machine is set to tandy.",
      {
         { "auto", NULL },
         { "on", "true" },
         { "off", "false" },
         { NULL, NULL },
      },
      "off"
   },
   {
      "dosbox_svn_disney",
      "Sound: Enable Disney Sound Source",
      "Enable Disney Sound Source Emulation.",
      {
         { "off", "false" },
         { "on", "true" },
         { NULL, NULL },
      },
      "off"
   },
   {
      "dosbox_svn_ipx",
      "Network: Enable IPX",
      "Enable IPX over UDP tunneling.",
      {
         { "false", NULL },
         { "true", NULL },
         { NULL, NULL },
      },
      "false"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};

/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL,           /* RETRO_LANGUAGE_FRENCH */
   NULL,           /* RETRO_LANGUAGE_SPANISH */
   NULL,           /* RETRO_LANGUAGE_GERMAN */
   NULL,           /* RETRO_LANGUAGE_ITALIAN */
   NULL,           /* RETRO_LANGUAGE_DUTCH */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_BRAZIL */
   NULL,           /* RETRO_LANGUAGE_PORTUGUESE_PORTUGAL */
   NULL,           /* RETRO_LANGUAGE_RUSSIAN */
   NULL,           /* RETRO_LANGUAGE_KOREAN */
   NULL,           /* RETRO_LANGUAGE_CHINESE_TRADITIONAL */
   NULL,           /* RETRO_LANGUAGE_CHINESE_SIMPLIFIED */
   NULL,           /* RETRO_LANGUAGE_ESPERANTO */
   NULL,           /* RETRO_LANGUAGE_POLISH */
   NULL,           /* RETRO_LANGUAGE_VIETNAMESE */
   NULL,           /* RETRO_LANGUAGE_ARABIC */
   NULL,           /* RETRO_LANGUAGE_GREEK */
   NULL,           /* RETRO_LANGUAGE_TURKISH */
};
#endif

/*
 ********************************
 * Functions
 ********************************
*/

/* Handles configuration/setting of core options.
 * Should be called as early as possible - ideally inside
 * retro_set_environment(), and no later than retro_load_game()
 * > We place the function body in the header to avoid the
 *   necessity of adding more .c files (i.e. want this to
 *   be as painless as possible for core devs)
 */

static INLINE void libretro_set_core_options(retro_environment_t environ_cb)
{
   unsigned version = 0;

   if (!environ_cb)
      return;

   if (environ_cb(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && (version >= 1))
   {
#ifndef HAVE_NO_LANGEXTRA
      struct retro_core_options_intl core_options_intl;
      unsigned language = 0;

      core_options_intl.us    = option_defs_us;
      core_options_intl.local = NULL;

      if (environ_cb(RETRO_ENVIRONMENT_GET_LANGUAGE, &language) &&
            (language < RETRO_LANGUAGE_LAST) && (language != RETRO_LANGUAGE_ENGLISH))
         core_options_intl.local = option_defs_intl[language];

      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl);
#else
      environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, &option_defs_us);
#endif
   }
   else
   {
      size_t i;
      size_t num_options               = 0;
      struct retro_variable *variables = NULL;
      char **values_buf                = NULL;

      /* Determine number of options */
      while (true)
      {
         if (option_defs_us[num_options].key)
            num_options++;
         else
            break;
      }

      /* Allocate arrays */
      variables  = (struct retro_variable *)calloc(num_options + 1, sizeof(struct retro_variable));
      values_buf = (char **)calloc(num_options, sizeof(char *));

      if (!variables || !values_buf)
         goto error;

      /* Copy parameters from option_defs_us array */
      for (i = 0; i < num_options; i++)
      {
         const char *key                        = option_defs_us[i].key;
         const char *desc                       = option_defs_us[i].desc;
         const char *default_value              = option_defs_us[i].default_value;
         struct retro_core_option_value *values = option_defs_us[i].values;
         size_t buf_len                         = 3;
         size_t default_index                   = 0;

         values_buf[i] = NULL;

         if (desc)
         {
            size_t num_values = 0;

            /* Determine number of values */
            while (true)
            {
               if (values[num_values].value)
               {
                  /* Check if this is the default value */
                  if (default_value)
                     if (strcmp(values[num_values].value, default_value) == 0)
                        default_index = num_values;

                  buf_len += strlen(values[num_values].value);
                  num_values++;
               }
               else
                  break;
            }

            /* Build values string */
            if (num_values > 0)
            {
               size_t j;

               buf_len += num_values - 1;
               buf_len += strlen(desc);

               values_buf[i] = (char *)calloc(buf_len, sizeof(char));
               if (!values_buf[i])
                  goto error;

               strcpy(values_buf[i], desc);
               strcat(values_buf[i], "; ");

               /* Default value goes first */
               strcat(values_buf[i], values[default_index].value);

               /* Add remaining values */
               for (j = 0; j < num_values; j++)
               {
                  if (j != default_index)
                  {
                     strcat(values_buf[i], "|");
                     strcat(values_buf[i], values[j].value);
                  }
               }
            }
         }

         variables[i].key   = key;
         variables[i].value = values_buf[i];
      }

      /* Set variables */
      environ_cb(RETRO_ENVIRONMENT_SET_VARIABLES, variables);

error:

      /* Clean up */
      if (values_buf)
      {
         for (i = 0; i < num_options; i++)
         {
            if (values_buf[i])
            {
               free(values_buf[i]);
               values_buf[i] = NULL;
            }
         }

         free(values_buf);
         values_buf = NULL;
      }

      if (variables)
      {
         free(variables);
         variables = NULL;
      }
   }
}

#ifdef __cplusplus
}
#endif

#endif
