// This is copyrighted software. More information is at the end of this file.
#pragma once
#include <exception>
#include <string>
#include <utility>
#include <variant>

namespace retro {

class CoreOptionValue final
{
public:
    CoreOptionValue(std::string value, std::string label = {}) noexcept;
    CoreOptionValue(const char* value, std::string label = {}) noexcept;
    CoreOptionValue(int value, std::string label = {}) noexcept;
    CoreOptionValue(bool value, std::string label = {}) noexcept;

    [[nodiscard]]
    auto isValid() const noexcept -> bool;

    [[nodiscard]]
    auto toString() const noexcept -> const std::string&;

    [[nodiscard]]
    auto toInt() const noexcept -> int;

    [[nodiscard]]
    auto toBool() const noexcept -> bool;

    [[nodiscard]]
    auto toFloat() const noexcept -> float;

    [[nodiscard]]
    auto label() const noexcept -> const std::string&;

    [[nodiscard]]
    auto operator ==(const CoreOptionValue& other) const noexcept -> bool;

private:
    using ValueType = std::variant<std::monostate, int, bool>;

    ValueType value_;
    std::string value_str_;
    std::string label_;

    CoreOptionValue(ValueType value, std::string label) noexcept;

    template <typename T>
    [[nodiscard]]
    auto value() const noexcept -> T;
};

inline CoreOptionValue::CoreOptionValue(std::string value, std::string label) noexcept
    : value_str_(std::move(value))
    , label_(std::move(label))
{}

inline CoreOptionValue::CoreOptionValue(const char* const value, std::string label) noexcept
    : CoreOptionValue(std::string(value), std::move(label))
{}

inline CoreOptionValue::CoreOptionValue(const int value, std::string label) noexcept
    : CoreOptionValue(ValueType(value), std::move(label))
{}

inline CoreOptionValue::CoreOptionValue(const bool value, std::string label) noexcept
    : CoreOptionValue(ValueType(value), std::move(label))
{}

inline auto CoreOptionValue::isValid() const noexcept -> bool
{
    return !toString().empty();
}

inline auto CoreOptionValue::toString() const noexcept -> const std::string&
{
    return value_str_;
}

inline auto CoreOptionValue::toInt() const noexcept -> int
{
    return value<int>();
}

inline auto CoreOptionValue::toBool() const noexcept -> bool
{
    return value<bool>();
}

inline auto CoreOptionValue::toFloat() const noexcept -> float
{
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_);
    }
    try {
        return std::stof(toString());
    }
    catch (...) {
        // TODO: log
        return 0;
    }
}

inline auto CoreOptionValue::label() const noexcept -> const std::string&
{
    return label_;
}

inline auto CoreOptionValue::operator ==(const CoreOptionValue& other) const noexcept -> bool
{
    if (std::holds_alternative<std::monostate>(value_)) {
        return toString() == other.toString();
    }
    return value_ == other.value_;
}

inline CoreOptionValue::CoreOptionValue(
        CoreOptionValue::ValueType value, std::string label) noexcept
    : value_(std::move(value))
    , label_(std::move(label))
{
    // FIXME: to_string() is locale-dependent. Use something else.
    // TODO: Ddd parameter for FP significant digits.
    if (std::holds_alternative<int>(value_)) {
        value_str_ = std::to_string(std::get<int>(value_));
    } else if (std::holds_alternative<bool>(value_)) {
        value_str_ = std::get<bool>(value_) ? "true" : "false";
    } else {
        // TODO: Can't happen, log as internal bug.
        std::terminate();
    }
}

template <typename T>
inline auto CoreOptionValue::value() const noexcept -> T
{
    if (std::holds_alternative<T>(value_)) {
        return std::get<T>(value_);
    }
    if (std::holds_alternative<int>(value_)) {
        return std::get<int>(value_);
    }
    if (std::holds_alternative<bool>(value_)) {
        return std::get<bool>(value_);
    }
    // TODO: log
    return {};
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
