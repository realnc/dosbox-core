// This is copyrighted software. More information is at the end of this file.
#include "fake_timing.h"
#include <chrono>
#include <thread>

static const auto zero_point = std::chrono::steady_clock::now();
static std::chrono::milliseconds fake_delay_ticks{};
static bool use_fake_timing = true;

void fakeDelay(const std::uint32_t ms) noexcept
{
    if (use_fake_timing) {
        fake_delay_ticks += std::chrono::milliseconds(ms);
    } else {
        std::this_thread::sleep_for(std::chrono::microseconds(ms));
    }
}

auto fakeGetTicks() noexcept -> std::uint32_t
{
    using namespace std::chrono;

    if (use_fake_timing) {
        const auto tick_now = steady_clock::now() - zero_point;
        return duration_cast<milliseconds>(tick_now + fake_delay_ticks).count();
    }
    return duration_cast<milliseconds>(steady_clock::now() - zero_point)
        .count();
}

void fakeTimingReset() noexcept
{
    fake_delay_ticks = {};
}

void enableFakeTiming(bool enable) noexcept
{
    use_fake_timing = enable;
}

/*

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
