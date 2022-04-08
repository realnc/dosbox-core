// This is copyrighted software. More information is at the end of this file.
#include "mixer_lock.h"
#include "log.h"
#include <mutex>

static std::recursive_mutex mixer_mutex;

void lockMixer()
{
    mixer_mutex.lock();
}

void unlockMixer()
{
    mixer_mutex.unlock();
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
