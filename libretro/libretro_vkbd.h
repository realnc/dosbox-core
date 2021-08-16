#ifndef LIBRETRO_VKBD_H
#define LIBRETRO_VKBD_H

#include "libretro_graph.h"

extern bool retro_vkbd;
extern bool retro_capslock;
extern void retro_key_down(const int keycode);
extern void retro_key_up(const int keycode);

extern void print_vkbd(void);
extern void input_vkbd(void);
extern void toggle_vkbd(void);

extern unsigned int opt_vkbd_theme;
extern libretro_graph_alpha_t opt_vkbd_alpha;

extern bool libretro_supports_bitmasks;
extern int16_t joypad_bits[5];

#define VKBDX 13
#define VKBDY 7

#if 0
#define POINTER_DEBUG
#endif
#ifdef POINTER_DEBUG
extern int pointer_x;
extern int pointer_y;
#endif

typedef struct
{
   char normal[10];
   char shift[10];
   int value;
} retro_vkeys;

static retro_vkeys vkeys[VKBDX * VKBDY * 2] =
{
   /* 0 PG1 */
   { "Esc" ,"Esc" ,RETROK_ESCAPE },
   { "F1"  ,"F1"  ,RETROK_F1 },
   { "F2"  ,"F2"  ,RETROK_F2 },
   { "F3"  ,"F3"  ,RETROK_F3 },
   { "F4"  ,"F4"  ,RETROK_F4 },
   { "F5"  ,"F5"  ,RETROK_F5 },
   { "F6"  ,"F6"  ,RETROK_F6 },
   { "F7"  ,"F7"  ,RETROK_F7 },
   { "F8"  ,"F8"  ,RETROK_F8 },
   { "F9"  ,"F9"  ,RETROK_F9 },
   { "F10" ,"F10" ,RETROK_F10 },
   { "F11" ,"F11" ,RETROK_F11 },
   { "F12" ,"F12" ,RETROK_F12 },

   /* 13 */
   { "`"   ,"~"   ,RETROK_BACKQUOTE },
   { "1"   ,"!"   ,RETROK_1 },
   { "2"   ,"@"   ,RETROK_2 },
   { "3"   ,"#"   ,RETROK_3 },
   { "4"   ,"$"   ,RETROK_4 },
   { "5"   ,"%"   ,RETROK_5 },
   { "6"   ,"^"   ,RETROK_6 },
   { "7"   ,"&"   ,RETROK_7 },
   { "8"   ,"*"   ,RETROK_8 },
   { "9"   ,"("   ,RETROK_9 },
   { "0"   ,")"   ,RETROK_0 },
   { "-"   ,"_"   ,RETROK_MINUS },
   { "="   ,"+"   ,RETROK_EQUALS },

   /* 26 */
   { {11}  ,{11}  ,RETROK_TAB },
   { "Q"   ,"Q"   ,RETROK_q },
   { "W"   ,"W"   ,RETROK_w },
   { "E"   ,"E"   ,RETROK_e },
   { "R"   ,"R"   ,RETROK_r },
   { "T"   ,"T"   ,RETROK_t },
   { "Y"   ,"Y"   ,RETROK_y },
   { "U"   ,"U"   ,RETROK_u },
   { "I"   ,"I"   ,RETROK_i },
   { "O"   ,"O"   ,RETROK_o },
   { "P"   ,"P"   ,RETROK_p },
   { "["   ,"{"   ,RETROK_LEFTBRACKET },
   { "]"   ,"}"   ,RETROK_RIGHTBRACKET },

   /* 39 */
   { "Caps\1Lock","Caps\1Lock",RETROK_CAPSLOCK },
   { "A"   ,"A"   ,RETROK_a },
   { "S"   ,"S"   ,RETROK_s },
   { "D"   ,"D"   ,RETROK_d },
   { "F"   ,"F"   ,RETROK_f },
   { "G"   ,"G"   ,RETROK_g },
   { "H"   ,"H"   ,RETROK_h },
   { "J"   ,"J"   ,RETROK_j },
   { "K"   ,"K"   ,RETROK_k },
   { "L"   ,"L"   ,RETROK_l },
   { ";"   ,":"   ,RETROK_SEMICOLON },
   { "'"   ,"\""  ,RETROK_QUOTE },
   { "\\"  ,"|"   ,RETROK_BACKSLASH },

   /* 52 */
   { {12}  ,{12}  ,RETROK_LSHIFT },
   { "<"   ,">"   ,RETROK_OEM_102 },
   { "Z"   ,"Z"   ,RETROK_z },
   { "X"   ,"X"   ,RETROK_x },
   { "C"   ,"C"   ,RETROK_c },
   { "V"   ,"V"   ,RETROK_v },
   { "B"   ,"B"   ,RETROK_b },
   { "N"   ,"N"   ,RETROK_n },
   { "M"   ,"M"   ,RETROK_m },
   { ","   ,"<"   ,RETROK_COMMA },
   { "."   ,">"   ,RETROK_PERIOD },
   { "/"   ,"?"   ,RETROK_SLASH },
   { {12}  ,{12}  ,RETROK_RSHIFT },

   /* 65 */
   { "Ctrl","Ctrl",RETROK_LCTRL },
   { "Alt" ,"Alt" ,RETROK_LALT },
   { {18}  ,{18}  ,RETROK_SPACE },
   { "Alt\1Gr","Alt\1Gr" ,RETROK_RALT },
   { "Ctrl","Ctrl",RETROK_RCTRL },
   { {}  ,{}  , -1 },
   { "Ins" ,"Ins" ,RETROK_INSERT },
   { "Home","Home",RETROK_HOME },
   { "Page\1Up","Page\1Up",RETROK_PAGEUP },
   { {}  ,{}  , -1 },
   { {25}  ,{25}  ,RETROK_BACKSPACE },
   { {30}  ,{30}  ,RETROK_UP },
   { {16}  ,{16}  ,RETROK_RETURN },

   /* 78 */
   { {15}  ,{15}  ,-2 }, /* Numpad */
   { "Prnt\1Scrn","Sys\1Rq",RETROK_PRINT }, 
   { "Scrl\1Lock","Scrl\1Lock",RETROK_SCROLLOCK },   
   { "Pse","Brk",RETROK_PAUSE },
   { "Num\1Lock","Num\1Lock",RETROK_NUMLOCK },
   { {}  ,{}  , -1 },
   { "Del" ,"Del" ,RETROK_DELETE },
   { "End" ,"End" ,RETROK_END },
   { "Page\1Down","Page\1Down" ,RETROK_PAGEDOWN },
   { {}  ,{}  , -1 },
   { {27}  ,{27}  ,RETROK_LEFT },
   { {28}  ,{28}  ,RETROK_DOWN },
   { {29}  ,{29}  ,RETROK_RIGHT },

   /* 0 PG2 */
   { "Esc" ,"Esc" ,RETROK_ESCAPE },
   { "F1"  ,"F1"  ,RETROK_F1 },
   { "F2"  ,"F2"  ,RETROK_F2 },
   { "F3"  ,"F3"  ,RETROK_F3 },
   { "F4"  ,"F4"  ,RETROK_F4 },
   { "F5"  ,"F5"  ,RETROK_F5 },
   { "F6"  ,"F6"  ,RETROK_F6 },
   { "F7"  ,"F7"  ,RETROK_F7 },
   { "F8"  ,"F8"  ,RETROK_F8 },
   { "F9"  ,"F9"  ,RETROK_F9 },
   { "F10" ,"F10" ,RETROK_F10 },
   { "F11" ,"F11" ,RETROK_F11 },
   { "F12" ,"F12" ,RETROK_F12 },

   /* 13 */
   { "`"     ,"~"    ,RETROK_BACKQUOTE },
   { {15,'1'},"End"  ,RETROK_KP1 },
   { {15,'2'},{15,28},RETROK_KP2 },
   { {15,'3'},"PgDn" ,RETROK_KP3 },
   { {15,'4'},{15,27},RETROK_KP4 },
   { {15,'5'}," "    ,RETROK_KP5 },
   { {15,'6'},{15,29},RETROK_KP6 },
   { {15,'7'},"Home" ,RETROK_KP7 },
   { {15,'8'},{15,30},RETROK_KP8 },
   { {15,'9'},"PgUp" ,RETROK_KP9 },
   { {15,'0'},"Ins"  ,RETROK_KP0 },
   { {15,'-'},{15,'-'},RETROK_KP_MINUS },
   { {15,'+'},{15,'+'},RETROK_KP_PLUS },

   /* 26 */
   { {11}  ,{11}  ,RETROK_TAB },
   { "Q"   ,"Q"   ,RETROK_q },
   { "W"   ,"W"   ,RETROK_w },
   { "E"   ,"E"   ,RETROK_e },
   { "R"   ,"R"   ,RETROK_r },
   { "T"   ,"T"   ,RETROK_t },
   { "Y"   ,"Y"   ,RETROK_y },
   { "U"   ,"U"   ,RETROK_u },
   { "I"   ,"I"   ,RETROK_i },
   { "O"   ,"O"   ,RETROK_o },
   { "P"   ,"P"   ,RETROK_p },
   { "["   ,"{"   ,RETROK_LEFTBRACKET },
   { "]"   ,"}"   ,RETROK_RIGHTBRACKET },

   /* 39 */
   { "Caps\1Lock","Caps\1Lock",RETROK_CAPSLOCK },
   { "A"   ,"A"   ,RETROK_a },
   { "S"   ,"S"   ,RETROK_s },
   { "D"   ,"D"   ,RETROK_d },
   { "F"   ,"F"   ,RETROK_f },
   { "G"   ,"G"   ,RETROK_g },
   { "H"   ,"H"   ,RETROK_h },
   { "J"   ,"J"   ,RETROK_j },
   { "K"   ,"K"   ,RETROK_k },
   { "L"   ,"L"   ,RETROK_l },
   { ";"   ,":"   ,RETROK_SEMICOLON },
   { "'"   ,"\""  ,RETROK_QUOTE },
   { {15,'*'},{15,'*'},RETROK_KP_MULTIPLY },   

   /* 52 */
   { {12}  ,{12}  ,RETROK_LSHIFT },
   { "<"   ,">"   ,RETROK_OEM_102 },
   { "Z"   ,"Z"   ,RETROK_z },
   { "X"   ,"X"   ,RETROK_x },
   { "C"   ,"C"   ,RETROK_c },
   { "V"   ,"V"   ,RETROK_v },
   { "B"   ,"B"   ,RETROK_b },
   { "N"   ,"N"   ,RETROK_n },
   { "M"   ,"M"   ,RETROK_m },
   { ","   ,"<"   ,RETROK_COMMA },
   { "."   ,">"   ,RETROK_PERIOD },
   { {15,'/'},{15,'/'},RETROK_KP_DIVIDE },
   { {12}  ,{12}  ,RETROK_RSHIFT },

   /* 65 */
   { "Ctrl","Ctrl",RETROK_LCTRL },
   { "Alt" ,"Alt" ,RETROK_LALT },
   { {18}  ,{18}  ,RETROK_SPACE },
   { "Alt\1Gr","Alt\1Gr" ,RETROK_RALT },
   { "Ctrl","Ctrl",RETROK_RCTRL },
   { {}  ,{}  , -1 },
   { {15,'0'},{15,'I','n','s'} ,RETROK_KP0 },
   { "Home","Home",RETROK_HOME },
   { "Page\1Up","Page\1Up",RETROK_PAGEUP },
   { {}  ,{}  , -1 },
   { {25}  ,{25}  ,RETROK_BACKSPACE },
   { {15,30} ,{15,30},RETROK_KP8 },
   { {15,16},{15,16},RETROK_KP_ENTER },

   /* 78 */
   { {15}  ,{15}  ,-2 }, /* Numpad */
   { "Prnt\1Scrn","Sys\1Rq",RETROK_PRINT }, 
   { "Scrl\1Lock","Scrl\1Lock",RETROK_SCROLLOCK },   
   { "Pse","Brk",RETROK_PAUSE },
   { "Num\1Lock","Num\1Lock",RETROK_NUMLOCK },
   { {}  ,{}  , -1 },
   { {15,'.'},{15,'D','e','l'},RETROK_KP_PERIOD },
   { "End" ,"End" ,RETROK_END },
   { "Page\1Down","Page\1Down" ,RETROK_PAGEDOWN },
   { {}  ,{}  , -1 },
   { {15,27},{15,27},RETROK_KP4 },
   { {15,28},{15,28},RETROK_KP2 },
   { {15,29},{15,29},RETROK_KP6 },
};

#endif /* LIBRETRO_VKBD_H */
