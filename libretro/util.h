// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <algorithm>
#include <cctype>
#include <string>

[[nodiscard]]
inline auto lower_case(std::string str) noexcept -> std::string
{
    for (auto& c : str) {
        c = std::tolower(c);
    }
    return str;
}

[[nodiscard]]
inline auto upper_case(std::string str) noexcept -> std::string
{
    for (auto& c : str) {
        c = std::toupper(c);
    }
    return str;
}

template <typename First, typename... T>
auto is_equal_to_one_of(const First& first, const T&... t) -> bool
{
    return ((first == t) || ...);
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
