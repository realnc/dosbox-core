#include <algorithm>
#include <libco.h>
#include <string.h>
#include "dosbox.h"
#include "libretro.h"
#include "libretro_dosbox.h"
#include "render.h"
#include "video.h"

Bit8u dosbox_framebuffers[2][1024 * 768 * 4] = { 0 };
Bit8u *dosbox_frontbuffer = dosbox_framebuffers[0];
static Bit8u *dosbox_backbuffer = dosbox_framebuffers[1];
bool dosbox_frontbuffer_uploaded = false;
Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
float dosbox_aspect_ratio = 0;
unsigned RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_0RGB1555;
static GFX_CallBack_t dosbox_gfx_cb = NULL;

Bitu GFX_GetBestMode(Bitu flags)
{
    return GFX_CAN_32 | GFX_RGBONLY;
}

Bitu GFX_GetRGB(Bit8u red,Bit8u green,Bit8u blue)
{
    return (red << 16) | (green << 8) | (blue << 0);
}

Bitu GFX_SetSize(Bitu width,Bitu height,Bitu flags,double scalex,double scaley,GFX_CallBack_t cb)
{
    memset(dosbox_framebuffers, 0, sizeof(dosbox_framebuffers));
    RDOSGFXwidth = width;
    RDOSGFXheight = height;
    RDOSGFXpitch = width * 4;
    dosbox_aspect_ratio = (width * scalex) / (height * scaley);
    dosbox_gfx_cb = cb;

    if(RDOSGFXwidth > 1024 || RDOSGFXheight > 768)
        return 0;

    return GFX_GetBestMode(0);
}

bool GFX_StartUpdate(Bit8u * & pixels,Bitu & pitch)
{
    pixels = (core_timing == CORE_TIMING_SYNCED) ? dosbox_framebuffers[0] : dosbox_backbuffer;
    pitch = RDOSGFXpitch;
    return true;
}

void GFX_EndUpdate( const Bit16u *changedLines )
{
    if (core_timing == CORE_TIMING_SYNCED)
    {
        video_cb(changedLines ? dosbox_framebuffers[0] : NULL, RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch);
    }
    else if (dosbox_frontbuffer_uploaded && changedLines)
    {
        std::swap(dosbox_frontbuffer, dosbox_backbuffer);
        dosbox_frontbuffer_uploaded = false;
        // Tell dosbox to draw the next frame completely, not just the scanlines that changed.
        dosbox_gfx_cb(GFX_CallBackRedraw);
    }
}

// Stubs
void GFX_SetTitle(Bit32s cycles, int frameskip, bool paused){}
void GFX_ShowMsg(char const* format,...){}
void GFX_Events(){}
void GFX_SetPalette(Bitu start,Bitu count,GFX_PalEntry * entries){}

