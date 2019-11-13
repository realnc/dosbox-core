[![ko-fi](https://www.ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/U7U117Y74)

# DOSBox-SVN libretro

Upstream port of DOSBox-SVN to libretro.

## Requirements

- SDL 1.2
- SDL net 1.2

### Setting up a build environment on Windows
To build on Windows we recommend MSYS2 (https://www.msys2.org/).
Install MSYS2, and follow these instructions:

**Update Environment**

Start the MSYS2 shell and run:

```bash
pacman --noconfirm -Sy
pacman --needed --noconfirm -S bash pacman pacman-mirrors msys2-runtime
```

Restart MSYS2 and run:

```bash
pacman --noconfirm -Su
```

**Install the Toolchain**

For 32-bit builds run:

```bash
pacman -S --noconfirm --needed wget git make mingw-w64-i686-toolchain mingw-w64-i686-ntldd mingw-w64-i686-zlib mingw-w64-i686-pkg-config mingw-w64-i686-SDL2 mingw-w64-i686-SDL mingw-w64-i686-SDL_net
```

For 64-bit builds run:

```bash
pacman -S --noconfirm --needed wget git make mingw-w64-x86_64-toolchain mingw-w64-x86_64-ntldd mingw-w64-x86_64-zlib mingw-w64-x86_64-pkg-config mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_net
```

### Setting up a build environment on Linux

You need to install the libsdl1.2 and libsdlnet1.2 development headers, refer to your distribution documentation for reference.
Alternatively you can build with `WITH_FAKE_SDL=1`

## Compilation
Clone the repository and update submodules

```bash
git clone git@git.retromods.org:dev/dosbox-svn.git
git submodule update --init
git fetch libretro
git checkout libretro
```

Now enter the repo directory and build:

```bash
cd dosbox-svn
cd libretro
make -j8
```

If you want to enable the dynarec:

```bash
make -j8 WITH_DYNAREC=$ABI
```

The valid ABI choices are `arm, oldarm, x86_64, x86, ppc`

## Features
- Up-to-date with the latest DOSBox-SVN trunk.
- Configure everything on the fly without relying on configuration files.
- Different timing methods to suit your usecase. Play your games at perfect sync without tearing on variable refresh rate monitors on supported frontends (**RetroArch**).
- DOSBox-SVN upstream file system overlay support [(VOGONS thread)](https://www.vogons.org/viewtopic.php?f=31&t=66009).
- Portability, DOSBox-SVN libretro is currently available on:
    - Android (with dynarec support)
    - Linux (with dynarec support)
    - Mac OSX (with dynarec support on x86/x64 hardware)
    - Switch (with dynarec support) (**RetroArch**)
    - Wii U (**RetroArch**)
    - Windows (with dynarec support)
- External MIDI support (Windows, Linux) (**RetroArch**)
- Mouse emulation via gamepad
- Emulate keyboard and gamepad with a single physical device (**RetroArch**)

## Future Ideas
- CD-ROM image automount
- IPX

## Usage
You can load exe, bat, iso, cue, and conf files directly

**Notes:**

- Standalone DOSBox hotkeys do not work, you need to use **libretro core options**. Check your libretro frontend documentation for more information.

- You can also insert disk media at runtime by using the **libretro disk control interface**. Check your libretro frontend documentation for more information.

- The SDL keymapper is not available, you can remap keyboard and gamepad buttons using the **libretro controllers API**. Check your libretro frontend documentation for more information.

- To use the overlay filesystem functionality enable it under core options. Once that is enabled, data changed on the session will be saved under your frontend's **save directory**, named after the **parent directory name** of your game.

For example assuming you load: `C:\Games\DOS\Commander Keen 5 - The Armageddon Machine (1991)\CKEEN5.exe`, your data will be saved in `RETROARCH SAVE DIRECTORY\Commander Keen 5 - The Armageddon Machine (1991)`. In this particular case the following files were created on that dir:

`CONFIG.CK5  SAVEGAM0.CK5  SAVEGAM1.CK5`

The overlay filesystem allows you to have portability for your save data, but it may have issues with some games so it's disabled by default.

---

##### Features marked (**RetroArch**) are features not currently tested or available on other libretro frontends

