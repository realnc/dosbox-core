# DOSBox-core

A libretro core of [DOSBox](https://www.dosbox.com) for use in
[RetroArch](https://www.retroarch.com) and other
[libretro frontends](https://www.libretro.com/index.php/powered-by-libretro).
DOSBox-core is kept up to date with the latest sources from DOSBox SVN trunk,
plus some additional features:

* [Nuked OPL3](https://nukeykt.retrohost.net)  
  A cycle-accurate OPL3 (YMF262) emulator.

* [Munt MT-32](https://github.com/munt/munt)  
  A high quality emulator of the Roland MT-32, CM-32L and LAPC-I MIDI modules.

* [FluidSynth](http://www.fluidsynth.org)  
  A MIDI software synthesizer that supports SF2/SF3/DLS/GIG soundfonts.

DOSBox-core was originally based on the https://github.com/libretro/dosbox-svn
libretro core, developed mainly by [Radius](https://github.com/fr500) as well
as other contributors (the full commit history has been preserved.)

## Requirements

- C++17 compiler
- SDL 1.2
- SDL_net 1.2

## Usage

You can load exe, bat, iso, cue, and conf files directly.
