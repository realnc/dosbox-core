// This is copyrighted software. More information is at the end of this file.
#include "CoreOptions.h"

#include <algorithm>

namespace retro {

auto CoreOptions::operator[](const std::string_view key) const -> const CoreOptionValue&
{
    const auto* option = this->option(key);
    if (!option) {
        // TODO: log
        return invalid_value_;
    }

    retro_variable var{option->key().c_str()};
    if (!env_cb_(RETRO_ENVIRONMENT_GET_VARIABLE, &var) || !var.value) {
        // TODO: log
        return option->defaultValue();
    }

    auto value = std::find_if(option->begin(), option->end(), [var_val = var.value](const auto& a) {
        return a.toString() == var_val;
    });
    if (value == option->end()) {
        // TODO: log
        return option->defaultValue();
    }
    return *value;
}

auto CoreOptions::changed() const noexcept -> bool
{
    bool updated = false;
    env_cb_(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated);
    return updated;
}

void CoreOptions::updateFrontend()
{
    unsigned version = 0;

    updateRetroOptions();
    env_cb_(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version);

    if (env_cb_(RETRO_ENVIRONMENT_GET_CORE_OPTIONS_VERSION, &version) && version >= 1) {
        retro_core_options_intl core_options_intl{retro_options_.data()};
        if (!env_cb_(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_INTL, &core_options_intl)) {
            env_cb_(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, retro_options_.data());
        }
    } else {
        // Frontend only supports legacy options.
        updateFrontendV0();
    }
}

void CoreOptions::setVisible(const std::string_view key, const bool visible) const noexcept
{
    const auto* opt = option(key);
    if (!opt) {
        // TODO: log
        return;
    }
    retro_core_option_display option_display{opt->key().c_str(), visible};
    env_cb_(RETRO_ENVIRONMENT_SET_CORE_OPTIONS_DISPLAY, &option_display);
}

void CoreOptions::setVisible(
    const std::initializer_list<const std::string_view> keys, const bool visible) const noexcept
{
    for (const auto key : keys) {
        setVisible(key, visible);
    }
}

void CoreOptions::setCurrentValue(std::string_view key, const CoreOptionValue& value)
{
    auto* option = this->option(key);
    if (!option) {
        return;
    }

    auto default_value = option->defaultValue();
    auto values = option->clearValues();
    option->setValues({"_bogus_"}, "_bogus_");
    updateFrontend();
    option->setValues(values, value);
    updateFrontend();
    option->setDefaultValue(default_value);
    updateFrontend();
}

void CoreOptions::updateRetroOptions()
{
    retro_options_.clear();
    retro_options_.reserve(options_.size() + 1);
    for (const auto& option : options_) {
        retro_core_option_definition def{};
        def.key = option.key().c_str();
        def.desc = option.desc().c_str();
        def.info = option.info().empty() ? nullptr : option.info().c_str();
        def.default_value =
            option.defaultValue().isValid() ? option.defaultValue().toString().c_str() : nullptr;
        int i = 0;
        for (const auto& value : option) {
            def.values[i].value = value.toString().c_str();
            def.values[i].label = value.label().empty() ? nullptr : value.label().c_str();
            ++i;
        }
        def.values[i] = {};
        retro_options_.push_back(std::move(def));
    }
    retro_options_.push_back({});
}

void CoreOptions::updateFrontendV0()
{
    std::vector<retro_variable> variables;
    std::vector<std::string> value_strings;
    variables.reserve(options_.size() + 1);
    value_strings.reserve(options_.size());

    for (const auto& option : options_) {
        // Build values string.
        if (option.size() > 0) {
            auto str = option.desc();
            str += "; ";

            // Default value goes first.
            str += option.defaultValue().toString();

            // Add remaining values.
            for (const auto& value : option) {
                if (&value != &option.defaultValue()) {
                    str += '|';
                    str += value.toString();
                }
            }
            value_strings.emplace_back(std::move(str));
        }
        variables.push_back({option.key().c_str(), value_strings.back().c_str()});
    }
    variables.push_back({});
    env_cb_(RETRO_ENVIRONMENT_SET_VARIABLES, variables.data());
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
