// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <string>
#include <tuple>
#include <vector>

enum class MidiStandard
{
    Generic,
    GM,
    GM2,
    GS,
    MT32,
    XG,
};

/* Returns a list of all available ALSA MIDI output ports. The tuple contains the port's MIDI
 * standard, the port, the client name and the port name.
 */
auto getAlsaMidiPorts()
    -> std::vector<std::tuple<MidiStandard, std::string, std::string, std::string>>;

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
