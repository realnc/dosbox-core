// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "libretro.h"
#include <chrono>
#include <string>

namespace retro::internal {

void showOsdImpl(
    retro_log_level log_level, const char* msg, std::chrono::milliseconds duration,
    int priority) noexcept;

inline void showOsdImpl(
    retro_log_level log_level, const std::string& msg, std::chrono::milliseconds duration,
    int priority) noexcept
{
    showOsdImpl(log_level, msg.data(), duration, priority);
}

} // namespace retro::internal

namespace retro {
using namespace std::chrono_literals;

void setMessageEnvCb(retro_environment_t cb);

inline void showOsdInfo(
    const std::string& msg, const std::chrono::milliseconds duration = 2s,
    const int priority = 50) noexcept
{
    internal::showOsdImpl(RETRO_LOG_INFO, msg, duration, priority);
}

inline void showOsdWarn(
    const std::string& msg, const std::chrono::milliseconds duration = 2s,
    const int priority = 50) noexcept
{
    internal::showOsdImpl(RETRO_LOG_WARN, msg, duration, priority);
}

inline void showOsdError(
    const std::string& msg, const std::chrono::milliseconds duration = 2s,
    const int priority = 50) noexcept
{
    internal::showOsdImpl(RETRO_LOG_ERROR, msg, duration, priority);
}

} // namespace retro

/*

Copyright (C) 2022 Nikos Chantziaras.

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
