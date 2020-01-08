// This is copyrighted software. More information is at the end of this file.
#include "fake_timing.h"

#include <chrono>

static std::chrono::milliseconds fake_delay_ticks{};

void fakeDelay(const std::uint32_t ms) noexcept
{
    fake_delay_ticks += std::chrono::milliseconds(ms);
}

auto fakeGetTicks() noexcept -> std::uint32_t
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(
        steady_clock::now().time_since_epoch() + fake_delay_ticks).count();
}

void fakeTimingReset() noexcept
{
    fake_delay_ticks = {};
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
