#include "dosbox.h"
#include "emu_thread.h"
#include "libretro.h"
#include "libretro_dosbox.h"
#include "libretro-vkbd.h"
#include "log.h"
#include "render.h"
#include "vga.h"
#include "video.h"
#include <algorithm>
#include <cstring>

std::array<std::vector<Bit8u>, 2> dosbox_framebuffers;
std::vector<Bit8u>* dosbox_frontbuffer = &dosbox_framebuffers[0];
static std::vector<Bit8u>* dosbox_backbuffer = &dosbox_framebuffers[1];
bool dosbox_frontbuffer_uploaded = false;
Bitu RDOSGFXwidth, RDOSGFXheight, RDOSGFXpitch;
float dosbox_aspect_ratio = 0;
unsigned RDOSGFXcolorMode = RETRO_PIXEL_FORMAT_0RGB1555;
static GFX_CallBack_t dosbox_gfx_cb = nullptr;
#ifdef WITH_PINHACK
bool request_VGA_SetupDrawing = false;
#endif

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
    for (auto& buf : dosbox_framebuffers) {
        std::fill(buf.begin(), buf.end(), 0);
    }
    RDOSGFXwidth = width;
    RDOSGFXheight = height;
    RDOSGFXpitch = width * 4;
    dosbox_aspect_ratio = (width * scalex) / (height * scaley);
    dosbox_gfx_cb = cb;

    if (RDOSGFXwidth > GFX_MAX_WIDTH || RDOSGFXheight > GFX_MAX_HEIGHT) {
        return 0;
    }

    const auto fb_size = RDOSGFXwidth * RDOSGFXheight * 4;
    if (fb_size > dosbox_framebuffers[0].size()) {
        retro::logDebug("Increasing max framebuffer size to {}x{}", RDOSGFXwidth, RDOSGFXheight);
        for (auto& buf : dosbox_framebuffers) {
            buf.resize(fb_size);
        }
    }
    return GFX_GetBestMode(0);
}

auto GFX_StartUpdate(Bit8u*& pixels, Bitu& pitch) -> bool
{
    pixels = run_synced ? dosbox_framebuffers[0].data() : dosbox_backbuffer->data();
    pitch = RDOSGFXpitch;
    return true;
}

void GFX_EndUpdate(const Bit16u* const changedLines)
{
    if (retro_vkbd) {
        dosbox_frontbuffer_uploaded = false;
        dosbox_gfx_cb(GFX_CallBackRedraw);

        if (!run_synced) {
            std::swap(dosbox_frontbuffer, dosbox_backbuffer);
        }
        return;
    }

#ifdef WITH_PINHACK
    if (request_VGA_SetupDrawing) {
        request_VGA_SetupDrawing = false;
        VGA_SetupDrawing(0);
    }
#endif

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
void GFX_SetTitle(Bit32s /*cycles*/, int /*frameskip*/, bool /*paused*/)
{ }

void GFX_ShowMsg(char const* /*format*/, ...)
{ }

void GFX_Events()
{ }

void GFX_SetPalette(Bitu /*start*/, Bitu /*count*/, GFX_PalEntry* /*entries*/)
{ }

auto GFX_LazyFullscreenRequested() -> bool
{
    return false;
}

void GFX_TearDown()
{ }

auto GFX_IsFullscreen() -> bool
{
    return false;
}

void GFX_UpdateSDLCaptureState()
{ }

void GFX_RestoreMode()
{ }

void GFX_SwitchLazyFullscreen(bool /*lazy*/)
{ }

void GFX_SwitchFullscreenNoReset()
{ }

void GFX_SetShader(const char* /*src*/)
{ }
