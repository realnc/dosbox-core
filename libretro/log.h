// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "libretro.h"
#include <filesystem>
#include <fmt/format.h>
#include <utility>

namespace retro {

namespace internal {
    extern retro_log_printf_t log_cb;
    extern retro_log_level log_level;
} // namespace internal

/* Set the libretro log callback to use. If this is never called, or called with a null argument,
 * stdout/stderr will be used for output.
 */
void setRetroLogCb(retro_log_printf_t cb);

void setLoggingLevel(const retro_log_level log_level);

template <typename... Args>
constexpr void logDebug(Args&&... args)
{
    if (internal::log_level > RETRO_LOG_DEBUG) {
        return;
    }
    internal::log_cb(
        RETRO_LOG_DEBUG, "[DOSBox] %s\n", fmt::format(std::forward<Args>(args)...).c_str());
}

template <typename... Args>
constexpr void logInfo(Args&&... args)
{
    if (internal::log_level > RETRO_LOG_INFO) {
        return;
    }
    internal::log_cb(
        RETRO_LOG_INFO, "[DOSBox] %s\n", fmt::format(std::forward<Args>(args)...).c_str());
}

template <typename... Args>
constexpr void logWarn(Args&&... args)
{
    if (internal::log_level > RETRO_LOG_WARN) {
        return;
    }
    internal::log_cb(
        RETRO_LOG_WARN, "[DOSBox] %s\n", fmt::format(std::forward<Args>(args)...).c_str());
}

template <typename... Args>
constexpr void logError(Args&&... args)
{
    if (internal::log_level > RETRO_LOG_ERROR) {
        return;
    }
    internal::log_cb(
        RETRO_LOG_ERROR, "[DOSBox] %s\n", fmt::format(std::forward<Args>(args)...).c_str());
}

} // namespace retro

/* Make std::filesystem::path formattable.
 */
template <>
struct fmt::formatter<std::filesystem::path>: formatter<std::string>
{
    template <typename FormatContext>
    auto format(const std::filesystem::path& path, FormatContext& ctx)
    {
        return formatter<std::string>::format(path.string(), ctx);
    }
};

/*

Copyright (C) 2020 Nikos Chantziaras <realnc@gmail.com>

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
