// This is copyrighted software. More information is at the end of this file.
#pragma once
#include "CoreOptionDefinition.h"
#include "libretro.h"
#include <initializer_list>
#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace retro {

/* Wraps the libretro core options API. Supports options API version 1, but if the frontend doesn't,
 * options are automatically converted to the old "version 0" format.
 *
 * Usage is fairly straightforward:
 *
 * CoreOptions core_options {
 *     // All option keys will get automatically prefixed with this.
 *     "core_name_",
 *
 *     // Core option definitions follow.
 *     {
 *         {
 *             // Option key. Will be automatically converted to "core_name_overclock".
 *             "overclock",
 *
 *             // Label.
 *             "Overclock CPU",
 *
 *             // Description. Frontends usually display this as small text below the label.
 *             // Specifying a description is optional and can be omitted.
 *             "Overclocks the emulated CPU. Might result in glitches with some games."
 *
 *             // Possible values and their labels. Can be any combination of string, bool and int.
 *             // The label string is optional and can be omitted.
 *             {
 *                 { false, "OFF" },
 *                 { true, "ON" },
 *             },
 *
 *             // Default value. Must be one of the previously specified values, otherwise the first
 *             // value will be used.
 *             false
 *         },
 *
 *         // Note how we omit the description here.
 *         {
 *             "frameskip",
 *             "Frame skipping",
 *             {
 *                 { 0, "None" },
 *                 { 1, "One frame" },
 *                 { 2, "Two frames" },
 *                 { 3, "Three frames" },
 *             },
 *             0
 *         },
 *
 *         // Note how we omit both a description as well as the values here. Read below on why
 *         // omitting values is sometimes useful.
 *         {
 *             "midi_device",
 *             "MIDI output device",
 *         }
 *     }
 * };
 *
 * In the above example, we didn't speficy any values for the "midi_device" option. This allows for
 * options where the values need to be constructed at run-time. In this case, we would get the list
 * of available MIDI devices from the operating system and then add them as values with something
 * like:
 *
 *     vector<CoreOptionValue> midiValues;
 *     for (...) {
 *         midiValues.push_back({midi_device_id, label});
 *     }
 *     midiValues.push_back("none");
 *
 *     core_options.option("midi_device")->setValues(std::move(midiValues), "none");
 *
 * You MUST add values for any options originally defined without them, and you must do so BEFORE
 * you submit the options to the frontend. Otherwise, the behavior is undefined.
 *
 * After creating your core options, you must set the frontend environment callback with
 * setEnvironmentCallback() and submit the core options to the frontend with updateFrontend(). This
 * should happen as early as possible. retro_set_environment() is the best place to do it.
 *
 * To query the frontend for the current value of an option, use the [] operator:
 *
 *     bool overclock = core_options["overclock"].toBool();
 *     int frames_to_skip = core_options["frameskip"].toInt();
 *     string midi_device = core_options["midi_device"].toString();
 *
 * Note that the omission of the key prefix when using functions of this class is only a
 * convenience. The actual CoreOptionDefinition instances contain the full key. For example:
 *
 *     cout << options.option("overclock")->key();
 *
 * will print "core_name_overclock", not "overclock".
 */
class CoreOptions final
{
public:
    CoreOptions(std::string key_prefix, std::vector<CoreOptionDefinition> options);

    /* Set the frontend environment callback.
     */
    void setEnvironmentCallback(retro_environment_t cb);

    /* Query frontend for the current value of the option corresponding to the specified key.
     * Returns the default value of the option if the query fails. Returns an invalid value if 'key'
     * doesn't correspond to an option.
     */
    [[nodiscard]]
    auto operator[](std::string_view key) const -> const CoreOptionValue&;

    /* Returns true if any values were changed by the frontend since the last query.
     */
    [[nodiscard]]
    auto changed() const noexcept -> bool;

    /* Returns a pointer to the CoreOptionDefinition for the given key. Returns null if no option
     * with that key exists.
     */
    [[nodiscard]]
    auto option(std::string_view key) -> CoreOptionDefinition*;

    [[nodiscard]]
    auto option(std::string_view key) const -> const CoreOptionDefinition*;

    /* Submit the options to the frontend. Should be called as early as possible - ideally inside
     * retro_set_environment(), and no later than retro_load_game().
     */
    void updateFrontend();

    /* Show/hide the specified option(s).
     */
    void setVisible(std::string_view key, bool visible) const noexcept;
    void setVisible(
        std::initializer_list<const std::string_view> keys, bool visible) const noexcept;

private:
    std::vector<CoreOptionDefinition> options_;
    std::map<std::string, CoreOptionDefinition*, std::less<>> options_map_;
    std::vector<retro_core_option_definition> retro_options_;
    std::string key_prefix_;
    // TODO: log when called.
    retro_environment_t env_cb_ = [](unsigned, void*) { return false; };
    CoreOptionValue invalid_value_{""};

    void updateRetroOptions();
    void updateFrontendV0();
};

inline CoreOptions::CoreOptions(std::string key_prefix, std::vector<CoreOptionDefinition> options)
    : options_(std::move(options))
    , key_prefix_(std::move(key_prefix))
{
    for (auto& option : options_) {
        options_map_[option.key()] = &option;
        option.setKey(key_prefix_ + option.key());
    }
}

inline void CoreOptions::setEnvironmentCallback(const retro_environment_t cb)
{
    env_cb_ = cb;
}

inline auto CoreOptions::option(const std::string_view key) -> CoreOptionDefinition*
{
    auto it = options_map_.find(key);
    return it != options_map_.end() ? it->second : nullptr;
}

inline auto CoreOptions::option(const std::string_view key) const -> const CoreOptionDefinition*
{
    auto it = options_map_.find(key);
    return it != options_map_.end() ? it->second : nullptr;
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
