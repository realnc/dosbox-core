#include "dosbox.h"
#include "emu_thread.h"
#include "libretro.h"
#include "libretro_dosbox.h"
#include "render.h"
#include "video.h"
#include <algorithm>
#include <cstring>

Bit8u dosbox_framebuffers[2][1024 * 768 * 4] = {{0}};
Bit8u *dosbox_frontbuffer = dosbox_framebuffers[0];
static Bit8u *dosbox_backbuffer = dosbox_framebuffers[1];
bool dosbox_frontbuffer_uploaded = false;
Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
float dosbox_aspect_ratio = 0;
unsigned RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_0RGB1555;
static GFX_CallBack_t dosbox_gfx_cb = nullptr;

auto GFX_GetBestMode(const Bitu /*flags*/) -> Bitu
{
    return GFX_CAN_32 | GFX_RGBONLY;
}

auto GFX_GetRGB(const Bit8u red, const Bit8u green, const Bit8u blue) -> Bitu
{
    return (red << 16) | (green << 8) | (blue << 0);
}

auto GFX_SetSize(
        const Bitu width, const Bitu height, const Bitu /*flags*/, const double scalex,
        const double scaley, const GFX_CallBack_t cb) -> Bitu
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

auto GFX_StartUpdate(Bit8u*& pixels, Bitu& pitch) -> bool
{
    pixels = run_synced ? dosbox_framebuffers[0] : dosbox_backbuffer;
    pitch = RDOSGFXpitch;
    return true;
}

void GFX_EndUpdate(const Bit16u* const changedLines)
{
    if (run_synced) {
        dosbox_frontbuffer_uploaded = !changedLines;
    } else if (dosbox_frontbuffer_uploaded && changedLines) {
        std::swap(dosbox_frontbuffer, dosbox_backbuffer);
        dosbox_frontbuffer_uploaded = false;
        // Tell dosbox to draw the next frame completely, not just the scanlines that changed.
        dosbox_gfx_cb(GFX_CallBackRedraw);
    }
}

// Stubs
void GFX_SetTitle(Bit32s /*cycles*/, int /*frameskip*/, bool /*paused*/) {}
void GFX_ShowMsg(char const* /*format*/,...) {}
void GFX_Events(){}
void GFX_SetPalette(Bitu /*start*/, Bitu /*count*/, GFX_PalEntry* /*entries*/) {}
