// This is copyrighted software. More information is at the end of this file.
#include "CoreOptionDefinition.h"

namespace retro {

void CoreOptionDefinition::setValues(
    std::vector<CoreOptionValue> values, const CoreOptionValue& default_value)
{
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == default_value) {
            default_value_index_ = i;
            break;
        }
    }
    values_ = std::move(values);
}

} // namespace retro

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
