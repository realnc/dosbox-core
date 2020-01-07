// This is copyrighted software. More information is at the end of this file.
#include "midi_alsa.h"

#include <alsa/asoundlib.h>
#include <memory>

auto getAlsaMidiPorts() -> std::vector<std::tuple<std::string, std::string, std::string>>
{
    snd_seq_client_info_t* client_info;
    snd_seq_port_info_t* port_info;
    std::vector<std::tuple<std::string, std::string, std::string>> port_list;
    const auto seq = [] {
        snd_seq_t* tmp = nullptr;
        snd_seq_open(&tmp, "default", SND_SEQ_OPEN_DUPLEX, SND_SEQ_NONBLOCK);
        return std::unique_ptr<snd_seq_t, decltype(&snd_seq_close)>(tmp, snd_seq_close);
    }();

    if (!seq) {
        // TODO: log
        return {};
    }

    snd_seq_client_info_alloca(&client_info);
    snd_seq_port_info_alloca(&port_info);
    snd_seq_client_info_set_client(client_info, -1);

    while (snd_seq_query_next_client(seq.get(), client_info) == 0) {
        const int client_num = snd_seq_client_info_get_client(client_info);

        snd_seq_port_info_set_client(port_info, client_num);
        snd_seq_port_info_set_port(port_info, -1);

        while (snd_seq_query_next_port(seq.get(), port_info) == 0) {
            const unsigned port_caps = snd_seq_port_info_get_capability(port_info);
            const unsigned port_type = snd_seq_port_info_get_type(port_info);

            if ((port_type & SND_SEQ_PORT_TYPE_MIDI_GENERIC)
                && (port_caps & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)))
            {
                auto port = std::to_string(snd_seq_client_info_get_client(client_info))
                    + ':' + std::to_string(snd_seq_port_info_get_port(port_info));
                auto client_name = snd_seq_client_info_get_name(client_info);
                auto port_name = snd_seq_port_info_get_name(port_info);
                port_list.emplace_back(
                    std::move(port), std::move(client_name), std::move(port_name));
            }
        }
    }
    return port_list;
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
