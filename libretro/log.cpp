// This is copyrighted software. More information is at the end of this file.
#include "log.h"

#include <cstdarg>
#include <cstdio>

static RETRO_CALLCONV void logFallbackCb(const retro_log_level level, const char* const fmt, ...)
{
    std::va_list va;

    va_start(va, fmt);
    vfprintf(level >= RETRO_LOG_WARN ? stderr : stdout, fmt, va);
    va_end(va);
}

namespace retro::internal {
retro_log_printf_t log_cb = logFallbackCb;
};

namespace retro {

void setRetroLogCb(const retro_log_printf_t cb)
{
    internal::log_cb = cb ? cb : logFallbackCb;
}

} // namespace retro

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
