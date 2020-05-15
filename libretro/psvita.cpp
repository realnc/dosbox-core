// This is copyrighted software. More information is at the end of this file.
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <psp2/io/stat.h>
#include <psp2/kernel/threadmgr.h>
#include <string_view>
#include <unistd.h>

/* Implementations for missing functions in devkitPro for Vita.
 */
extern "C" {
auto chdir(const char* const /*path*/) -> int
{
    errno = EIO;
    return -1;
}

auto chmod(const char* const /*pathname*/, mode_t /*mode*/) -> int
{
    errno = EIO;
    return -1;
}

auto mkdir(const char* const dir, mode_t mode) -> int
{
    return sceIoMkdir(dir, mode);
}

auto rmdir(const char* const dir) -> int
{
    return sceIoRmdir(dir);
}

auto getcwd(char* buffer, size_t len) -> char*
{
    constexpr std::string_view cwd{"ux0:/data/retroarch/"};
    std::unique_ptr<char, decltype(&std::free)> buffer_raii{nullptr, std::free};

    if (buffer != nullptr && len == 0) {
        errno = EINVAL;
        return nullptr;
    }

    // Allocate a buffer if null is passed. This is a glibc extension, but it seems libstdc++ relies
    // on it.
    if (!buffer) {
        if (len == 0) {
            len = cwd.size() + 1;
        }
        buffer_raii.reset(static_cast<char*>(std::malloc(len)));
        if (!buffer_raii) {
            errno = ENOMEM;
            return nullptr;
        }
        buffer = buffer_raii.get();
    }

    if (len <= cwd.size()) {
        errno = ERANGE;
        return nullptr;
    }

    std::memcpy(buffer, cwd.data(), cwd.size());
    buffer[cwd.size()] = '\0';
    buffer_raii.release();
    return buffer;
}

auto pathconf(const char* const /*path*/, const int /*name*/) -> long
{
    errno = EINVAL;
    return -1;
}

auto usleep(const useconds_t usec) -> int
{
    sceKernelDelayThreadCB(usec);
    return 0;
}

auto sleep(const unsigned int seconds) -> unsigned int
{
    sceKernelDelayThreadCB(seconds * 1000000);
    return 0;
}
}

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
