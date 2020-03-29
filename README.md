# DOSBox-core

A libretro core of [DOSBox](https://www.dosbox.com) for use in
[RetroArch](https://www.retroarch.com) and other
[libretro frontends](https://www.libretro.com/index.php/powered-by-libretro).
DOSBox-core is kept up to date with the latest sources from DOSBox SVN trunk.

The core provides some improvements over the DOSBox-SVN core:

* Native MIDI support on Linux and Windows.  
  You can select the MIDI output port in the core settings. The core will try
  to select the correct MIDI device even if the MIDI port changes in the
  future, as it remembers the MIDI port by name, not by number.

* Cycle-accurate OPL3 (YMF262) emulation.  
  Using [Nuked OPL3](https://nukeykt.retrohost.net).

* MT-32, CM32-L and LAPC-I emulation.  
  Using [Munt](https://github.com/munt/munt).

* Soundfont-based MIDI synthesizer.  
  Using [FluidSynth](http://www.fluidsynth.org), a MIDI software synthesizer
  that supports SF2/SF3/DLS/GIG soundfonts.

* Other general, under-the-hood improvements and bugfixes.

DOSBox-core was originally based on the https://github.com/libretro/dosbox-svn
libretro core, developed mainly by [Radius](https://github.com/fr500) as well
as other contributors (the full commit history has been preserved.)

## Usage

You can load `.exe`, `.bat`, `.iso`, `.cue`, and `.conf` files directly. Note
that when loading content, the current directory is set to the content's
directory. This means it is possible to use relative paths in your `mount` and
`imgmount` commands.

For MT-32 emulation, make sure you have the correct MT-32 ROMs in your
frontend's system directory. See the `dosbox_core_libretro.info` file for the
ROM filenames and MD5 checksums.

## Compilation

Building the core requires a C++17 compiler. GCC 9 or newer is known to work.

CMake and Ninja are also assumed to be installed in order to build the bundled
dependencies.

Most of the dependencies are bundled. They are built and linked statically.
See `Makefile.libretro` for which make variables to set to disable the bundled
libraries and use system-installed libraries instead.

The only dependencies that are not bundled are glib and alsa-lib, the latter
only being needed on Linux.

Note that the bundled dependencies are provided in the form of git submodules,
so prior to building the core, you should first perform a:

    git submodule update --init

To build on Linux x86-64 with the bundled audio, libsndfile, SDL and SDL_net
libraries, you would do:

    cd libretro
    make BUNDLED_AUDIO_CODECS=1 BUNDLED_LIBSNDFILE=1 BUNDLED_SDL=1 WITH_DYNAREC=x86_64 deps
    make BUNDLED_AUDIO_CODECS=1 BUNDLED_LIBSNDFILE=1 BUNDLED_SDL=1 WITH_DYNAREC=x86_64 -j`nproc`

The first `make` invokation will build the bundled dependencies, the second
will built the core itself. You can omit the first invokation and the deps
will be built regardless, but doing it this way results in a faster build.

Note that the `clean` target will clean both dependency
and core object files. To clean just core objects, use the `targetclean`
target. To clean deps only, use `depsclean`.

To specify which compiler to use, set the `CC` and `CXX` environment
variables. For example:

    CC=gcc-9 CXX=g++-9 make ...

If you want to cross-compile, you should set `TARGET_TRIPLET` to the prefix
string of your cross compilation toolchain. For example:

    make TARGET_TRIPLET="i686-w64-mingw32.static" ...

If you want to use a different CMake generator when building dependencies that
use CMake, set the `CMAKE_GENERATOR` variable. By default, it's set to `Ninja`.
