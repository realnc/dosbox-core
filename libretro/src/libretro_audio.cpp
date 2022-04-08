// This is copyrighted software. More information is at the end of this file.
#include "libretro_audio.h"
#include "CoreOptions.h"
#include "SDL_stdinc.h"
#include "libretro.h"
#include "libretro_core_options.h"
#include "libretro_dosbox.h"
#include "log.h"
#include "mixer.h"
#include "mixer_lock.h"
#include <atomic>
#include <vector>

namespace audio {

static retro_audio_sample_batch_t batch_cb;
static std::vector<int16_t> buffer;

} // namespace audio

static void upload_async_audio(const int16_t* audio_data, int len_frames) noexcept
{
    while (len_frames > 0) {
        const auto uploaded_frames = audio::batch_cb(audio_data, len_frames);
        audio_data += uploaded_frames * 2;
        len_frames -= uploaded_frames;
    }
}

RETRO_CALLCONV void audioCallback()
{
    std::array<int16_t, 256> samples{};
    const auto available_frames = std::min(MIXER_RETRO_GetAvailableFrames(), samples.size() / 2);

    // RetroArch calls this every millisecond or so rather than only when it actually needs audio.
    // To avoid excessive mutex locking, we only lock if there's actually any samples in the mixer.
    if (available_frames == 0) {
        return;
    }

    MixerLocker mixer_locker;
    MIXER_CallBack(nullptr, (Uint8*)samples.data(), available_frames * 4);
    if (throttle_state.load(std::memory_order_relaxed).mode != RETRO_THROTTLE_SLOW_MOTION) {
        // Uploading audio inside the critical section results in video being synced to audio, since
        // the dosbox mixer acquires the same mutex as it writes audio. We normally don't want this,
        // unless slow motion is active in the frontend. In slow motion mode, the video would run at
        // normal speed while the audio would play in slow motion.
        mixer_locker.unlock();
    }
    upload_async_audio(samples.data(), available_frames);
}

static RETRO_CALLCONV void audioSetStateCallback(const bool /*enabled*/)
{
}

void init_audio()
{
    audio::buffer.reserve(4096);
    if (retro::core_options[CORE_OPT_ASYNC_AUDIO].toBool()) {
        retro_audio_callback audio_callback_handle{audioCallback, audioSetStateCallback};
        use_async_audio = environ_cb(RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK, &audio_callback_handle);
        if (!use_async_audio) {
            // TODO: OSD warning
        }
    }
}

auto queue_audio() -> Bitu
{
    const auto available_audio_frames = MIXER_RETRO_GetAvailableFrames();

    if (available_audio_frames > 0) {
        const auto samples = available_audio_frames * 2;
        const auto bytes = available_audio_frames * 4;

        if (samples > audio::buffer.capacity()) {
            audio::buffer.reserve(samples);
            retro::logDebug("Output audio buffer resized to {} samples.\n", samples);
        }
        MIXER_CallBack(nullptr, reinterpret_cast<Uint8*>(audio::buffer.data()), bytes);
    }
    return available_audio_frames;
}

void upload_audio(int len_frames) noexcept
{
    auto* src = audio::buffer.data();

    while (len_frames > 0) {
        const auto uploaded_frames = audio::batch_cb(src, len_frames);
        src += uploaded_frames * 2;
        len_frames -= uploaded_frames;
    }
}

void retro_set_audio_sample(retro_audio_sample_t)
{ }

void retro_set_audio_sample_batch(const retro_audio_sample_batch_t cb)
{
    audio::batch_cb = cb;
}

/*

Copyright (C) 2022 Nikos Chantziaras <realnc@gmail.com>

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
