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

/*
struct retro_variable vars_advanced[] = {
#if defined(C_DYNREC) || defined(C_DYNAMIC_X86)
    { "dosbox_svn_cpu_core",                "CPU core; auto|dynamic|normal|simple" },
#else
    { "dosbox_svn_cpu_core",                "CPU core; auto|normal|simple" },
#endif
    { "dosbox_svn_cpu_type",                "CPU type; auto|386|386_slow|486|486_slow|pentium_slow|386_prefetch" },
    { "dosbox_svn_cpu_cycles_mode",         "CPU cycle mode; auto|fixed|max" },
    { "dosbox_svn_cpu_cycles_multiplier",   "CPU cycle multiplier; 1000|10000|100000|100" },
    { "dosbox_svn_cpu_cycles",              "CPU cycles; 1|2|3|4|5|6|7|8|9" },
    { "dosbox_svn_cpu_cycles_multiplier_fine",
                                            "CPU fine cycles multiplier; 1000|1|10|100" },
    { "dosbox_svn_cpu_cycles_fine",         "CPU fine cycles; 1|2|3|4|5|6|7|9" },
    { "dosbox_svn_scaler",                  "Video scaler; none|normal2x|normal3x|advmame2x|advmame3x|advinterp2x|advinterp3x|hq2x|hq3x|2xsai|super2xsai|supereagle|tv2x|tv3x|rgb2x|rgb3x|scan2x|scan3x" },
    { "dosbox_svn_use_native_refresh",      "Refresh rate switching; false|true"},
    { "dosbox_svn_joystick_timed",          "Joystick timed intervals; true|false" },
    { "dosbox_svn_emulated_mouse",          "Gamepad emulated mouse; enable|disable" },
    { "dosbox_svn_emulated_mouse_deadzone", "Gamepad emulated deadzone; 5%|10%|15%|20%|25%|30%|0%" },
    { "dosbox_svn_mouse_speed_factor",      "Mouse speed; 1.00|1.25|1.50|1.75|2.00|2.25|2.50|2.75|3.00|3.25|3.50|3.75|4.00|4.25|4.50|4.75|5.00|0.25|0.50|0.75" },
    { "dosbox_svn_sblaster_type",           "Sound Blaster type; sb16|sb1|sb2|sbpro1|sbpro2|gb|none" },
    { "dosbox_svn_sblaster_base",           "Sound Blaster base address; 220|240|260|280|2a0|2c0|2e0|300" },
    { "dosbox_svn_sblaster_irq",            "Sound Blaster IRQ; 5|7|9|10|11|12|3" },
    { "dosbox_svn_sblaster_dma",            "Sound Blaster DMA; 1|3|5|6|7|0" },
    { "dosbox_svn_sblaster_hdma",           "Sound Blaster High DMA; 7|0|1|3|5|6" },
    { "dosbox_svn_sblaster_opl_mode",       "Sound Blaster OPL Mode; auto|cms|opl2|dualopl2|opl3|opl3gold|none" },
    { "dosbox_svn_sblaster_opl_emu",        "Sound Blaster OPL Provider; default|compat|fast|mame" },
    { "dosbox_svn_midi",                    "Enable MIDI passthrough; false|true" },
    { "dosbox_svn_pcspeaker",               "Enable PC-Speaker; false|true" },
    { "dosbox_svn_tandy",                   "Enable Tandy Sound System (restart); auto|on|off" },
    { "dosbox_svn_disney",                  "Enable Disney Sound Source (restart); false|true" },
#if defined(C_IPX)
    { "dosbox_svn_ipx",                     "Enable IPX over UDP; false|true" },
#endif
    { NULL, NULL },
};
*/


struct retro_core_option_definition option_defs_us[] = {
   {
      "dosbox_svn_use_options",
      "Core: Enable options (restart)",
      "Enable options. Disable in-case of using pre-generated configuration files.",
      {
         { "true", NULL },
         { "false", NULL },
         { NULL, NULL },
      },
      "true"
   },
   {
      "dosbox_svn_adv_options",
      "Core advanced options (restart)",
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
      "System: Enable overlay file system (restart)",
      "Enable overlay file system to redirect filesystem changes to the save directory. Disable if you have problems starting some games.",
      {
         { "true", NULL },
         { "false", NULL },
         { NULL, NULL },
      },
      "false"
   },
   {
      "dosbox_svn_machine_type",
      "Core: Emulated machine (restart)",
      "The type of machine that DOSBox will try to emulate.",
      {
         { "hercules", NULL },
         { "cga", NULL },
         { "tandy", NULL },
         { "pcjr", NULL },
         { "ega", NULL },
         { "vgaonly", NULL },
         { "svga_s3", NULL },
         { "svga_et3000", NULL },
         { "svga_et4000", NULL },
         { "svga_paradise", NULL },
         { "vesa_nolfb", NULL },
         { "vesa_oldvbe", NULL },
         { NULL, NULL },
      },
      "svga_s3"
   },
   {
      "dosbox_svn_memory_size",
      "System: Memory size (restart)",
      "The amount of memory that the emulated machine has.",
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
      "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core DYNREC available",
#else
      "CPU core used for emulation. Auto will switch to dynamic if appropiate. Dynamic core DYNAMIC_X86 available",
#endif
      {
         { "auto", NULL },
         { "dynamic", NULL },
         { "normal", NULL },
         { "simple", NULL },
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
         { "auto", NULL },
         { "normal", NULL },
         { "simple", NULL },
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
         { "auto", NULL },
         { "386", NULL },
         { "386_slow", NULL },
         { "386_prefetch", NULL },
         { "486", NULL },
         { "486_slow", NULL },
         { "pentium_slow", NULL },
         { NULL, NULL },
      },
      "auto"
   },
   {
      "dosbox_svn_cpu_cycles_mode",
      "System: CPU cycles mode",
      "Method to determine the amount of CPU cycles that DOSBox tries to emulate per milisecond. Use auto unless you have performance problems. A value that is too high for your system may cause slowdown.",
      {
         { "auto", NULL },
         { "fixed", NULL },
         { "max", NULL },
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
      "1000"
   },
   {
      "dosbox_svn_cpu_cycles",
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
      "dosbox_svn_cpu_cycles",
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
      "Video: Enable efresh rate switching",
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
      "Enable timed intervals for joystick axes. Experiment with this option if your joystick drifts.",
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
      "dosbox_svn_mouse_speed_factor",
      "Input: Gamepad emulated mouse speed",
      "Speed of the gamepad emulated mouse. Experiment with this value if the cursor moves too fast or too slow with your gamepad.",
      {
         { "1.00", NULL },
         { "1.25", NULL },
         { "1.50", NULL },
         { "1.75", NULL },
         { "2.00", NULL },
         { "2.25", NULL },
         { "2.50", NULL },
         { "2.75", NULL },
         { "3.00", NULL },
         { "3.25", NULL },
         { "3.50", NULL },
         { "3.75", NULL },
         { "4.00", NULL },
         { "4.25", NULL },
         { "4.50", NULL },
         { "4.75", NULL },
         { "5.00", NULL },
         { NULL, NULL },
      },
      "1.00"
   },
   {
      "dosbox_svn_sblaster_type",
      "Sound: SoundBlaster type",
      "Type of emulated SoundBlaster card.",
      {
         { "sb1", "SoundBlaster 1.0" },
         { "sb2", "SoundBlaster 2.0" },
         { "sbpro1", "SoundBlaster Pro" },
         { "sbpro2", "SoundBlaster Pro 2" },
         { "sb16", "SoundBlaster 16" },
         { "gb", "GameBlaster" },
         { NULL, NULL },
      },
      "sb16"
   },
   { NULL, NULL, NULL, {{0}}, NULL },
};




/*
    { "dosbox_svn_joystick_timed",          "Joystick timed intervals; true|false" },
    { "dosbox_svn_emulated_mouse",          "Gamepad emulated mouse; enable|disable" },
    { "dosbox_svn_emulated_mouse_deadzone", "Gamepad emulated deadzone; 5%|10%|15%|20%|25%|30%|0%" },
    { "dosbox_svn_mouse_speed_factor",      "Mouse speed; 1.00|1.25|1.50|1.75|2.00|2.25|2.50|2.75|3.00|3.25|3.50|3.75|4.00|4.25|4.50|4.75|5.00|0.25|0.50|0.75" },
    { "dosbox_svn_sblaster_type",           "Sound Blaster type; sb16|sb1|sb2|sbpro1|sbpro2|gb|none" },
    { "dosbox_svn_sblaster_base",           "Sound Blaster base address; 220|240|260|280|2a0|2c0|2e0|300" },
    { "dosbox_svn_sblaster_irq",            "Sound Blaster IRQ; 5|7|9|10|11|12|3" },
    { "dosbox_svn_sblaster_dma",            "Sound Blaster DMA; 1|3|5|6|7|0" },
    { "dosbox_svn_sblaster_hdma",           "Sound Blaster High DMA; 7|0|1|3|5|6" },
    { "dosbox_svn_sblaster_opl_mode",       "Sound Blaster OPL Mode; auto|cms|opl2|dualopl2|opl3|opl3gold|none" },
    { "dosbox_svn_sblaster_opl_emu",        "Sound Blaster OPL Provider; default|compat|fast|mame" },
    { "dosbox_svn_midi",                    "Enable MIDI passthrough; false|true" },
    { "dosbox_svn_pcspeaker",               "Enable PC-Speaker; false|true" },
    { "dosbox_svn_tandy",                   "Enable Tandy Sound System (restart); auto|on|off" },
    { "dosbox_svn_disney",                  "Enable Disney Sound Source (restart); false|true" },
#if defined(C_IPX)
    { "dosbox_svn_ipx",                     "Enable IPX over UDP; false|true" },
#endif
*/


/*
 ********************************
 * Language Mapping
 ********************************
*/

#ifndef HAVE_NO_LANGEXTRA
struct retro_core_option_definition *option_defs_intl[RETRO_LANGUAGE_LAST] = {
   option_defs_us, /* RETRO_LANGUAGE_ENGLISH */
   NULL,           /* RETRO_LANGUAGE_JAPANESE */
   NULL, /* RETRO_LANGUAGE_FRENCH */
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
   NULL, /* RETRO_LANGUAGE_TURKISH */
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
