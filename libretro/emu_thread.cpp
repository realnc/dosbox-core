// This is copyrighted software. More information is at the end of this file.
#include "emu_thread.h"

#include "libretro_dosbox.h"
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

static const auto main_thread_id = std::this_thread::get_id();
static bool use_spinlock = false;

// For CV-based waiting.
static bool emu_keep_waiting = true;
static bool main_keep_waiting = true;
static std::condition_variable emu_cv;
static std::condition_variable main_cv;
static std::mutex emu_mutex;
static std::mutex main_mutex;

// For spinning.
static std::atomic_flag emu_lock = ATOMIC_FLAG_INIT;
static std::atomic_flag main_lock = ATOMIC_FLAG_INIT;

static void switchToEmuWait()
{
    std::unique_lock emu_lock(emu_mutex);
    emu_keep_waiting = false;
    if (dosbox_exit) {
        emu_lock.unlock();
        emu_cv.notify_one();
        return;
    }
    emu_lock.unlock();
    std::unique_lock main_lock(main_mutex);
    emu_cv.notify_one();
    if (frontend_exit) {
        return;
    }
    main_keep_waiting = true;
    main_cv.wait(main_lock, []{ return !main_keep_waiting; });
}

static void switchToMainWait()
{
    std::unique_lock main_lock(main_mutex);
    main_keep_waiting = false;
    if (frontend_exit) {
        throw 1;
    }
    main_lock.unlock();
    std::unique_lock emu_lock(emu_mutex);
    main_cv.notify_one();
    emu_keep_waiting = true;
    emu_cv.wait(emu_lock, []{ return !emu_keep_waiting; });
}

static void switchToEmuSpin()
{
    main_lock.test_and_set(std::memory_order_acquire);
    emu_lock.clear(std::memory_order_release);
    std::this_thread::yield();
    while (main_lock.test_and_set(std::memory_order_acquire)) {
        if (dosbox_exit || frontend_exit) {
            return;
        }
    }
}

static void switchToMainSpin()
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

static void switchToEmuThread()
{
    const bool was_using_spinlock = use_spinlock;

    if (use_spinlock) {
        switchToEmuSpin();
    } else {
        switchToEmuWait();
    }
    if (was_using_spinlock != use_spinlock) {
        if (use_spinlock) {
            switchToEmuSpin();
        } else {
            switchToEmuWait();
        }
    }
}

static void switchToMainThread()
{
    const bool was_using_spinlock = use_spinlock;

    if (use_spinlock) {
        switchToMainSpin();
    } else {
        switchToMainWait();
    }
    if (was_using_spinlock != use_spinlock) {
        if (use_spinlock) {
            switchToMainSpin();
        } else {
            switchToMainWait();
        }
    }
}

void switchThread()
{
    if (std::this_thread::get_id() == main_thread_id) {
        switchToEmuThread();
    } else {
        switchToMainThread();
    }
}

void useSpinlockThreadSync(const bool use_spinlock)
{
    if (use_spinlock == ::use_spinlock) {
        return;
    }

    ::use_spinlock = use_spinlock;

    if (std::this_thread::get_id() == main_thread_id) {
        if (use_spinlock) {
            std::unique_lock emu_lock(emu_mutex);
            emu_keep_waiting = false;
            emu_lock.unlock();
            emu_cv.notify_one();
        } else {
            emu_lock.clear(std::memory_order_release);
        }
    } else {
        if (use_spinlock) {
            std::unique_lock main_lock(emu_mutex);
            main_keep_waiting = false;
            main_lock.unlock();
            main_cv.notify_one();
        } else {
            main_lock.clear(std::memory_order_release);
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
