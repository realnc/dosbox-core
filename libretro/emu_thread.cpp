// This is copyrighted software. More information is at the end of this file.
#include "emu_thread.h"

#include "libretro_dosbox.h"
#include <atomic>
#include <thread>

static std::atomic_flag emu_lock = ATOMIC_FLAG_INIT;
static std::atomic_flag main_lock = ATOMIC_FLAG_INIT;

void switchToEmuThread()
{
    main_lock.test_and_set(std::memory_order_acquire);
    emu_lock.clear(std::memory_order_release);
    std::this_thread::yield();
    while (main_lock.test_and_set(std::memory_order_acquire)) {
        if (dosbox_exit) {
            return;
        }
    }
}

void switchToMainThread()
{
    emu_lock.test_and_set(std::memory_order_acquire);
    main_lock.clear(std::memory_order_release);
    std::this_thread::yield();
    while (emu_lock.test_and_set(std::memory_order_acquire)) {
        if (frontend_exit) {
            throw 1;
        }
    }
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
