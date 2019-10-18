# DOSBox-SVN libretro

Upstream port of DOSBox to libretro.

## Keeping up-to-date
Clone the repo from the svn upstream:

```bash
git svn clone svn://svn.code.sf.net/p/dosbox/code-0/dosbox/ dosbox-svn
```

This takes a few hours. Then add a remote for the git repo:

```bash
cd dosbox-svn
git remote add origin git@git.retromods.org:dev/dosbox-svn.git --> replace this with your repo URL if you're working in a fork
git fetch
git checkout master
git svn rebase
```

Switch to libretro branch and work there instead, don't make any changes to master:

```
git checkout libretro
git rebase master
```

Do your work, resolve conflicts if any, and then:
```bash
git push --force
```

## Compilation

### Requirements

- SDL1.2
- SDL_net

#### Windows

To build on Windows we recommend MSYS2 (https://www.msys2.org/).
Install MSYS2, and follow these instructions:

##### Update Environment

Start the MSYS2 shell and run:

```bash
pacman --noconfirm -Sy
pacman --needed --noconfirm -S bash pacman pacman-mirrors msys2-runtime
```

Restart MSYS2 and run:

```bash
pacman --noconfirm -Su
```

##### Install the Toolchain

For 32-bit builds run:

```bash
pacman -S --noconfirm --needed wget git make mingw-w64-i686-toolchain mingw-w64-i686-ntldd mingw-w64-i686-zlib mingw-w64-i686-pkg-config mingw-w64-i686-SDL2 mingw-w64-i686-SDL mingw-w64-i686-SDL_net
```

For 64-bit builds run:

```bash
pacman -S --noconfirm --needed wget git make mingw-w64-x86_64-toolchain mingw-w64-x86_64-ntldd mingw-w64-x86_64-zlib mingw-w64-x86_64-pkg-config mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_net
```

#### Linux

You need to install the libsdl1.2 and libsdlnet1.2 development headers, refer to your distribution documentation for reference

### Building

Clone the repository
```bash
git clone git@git.retromods.org:dev/dosbox-svn.git
git fetch libretro
git checkout libretro
```

Now enter the repo directory and build

```bash
cd dosbox-svn
cd libretro
make -j4
```

If you want to enable the dynarec:

```bash
make -j8 WITH_DYNAREC=$ABI
```

The valid ABI choices are `arm, oldarm, x86_64, x86, ppc, mips`

## Usage

You can load exe, bat, iso, cue, and conf files directly

DOSBox hotkeys do not work, you need to use **libretro core options**. Check your libretro frontend documentation for more information.

You can also insert disk media at runtime by using the **libretro disk control interface**. Check your libretro frontend documentation for more information.

The SDL keymapper is not available, you can remap keyboard and gamepad buttons using the **libretro controllers API**. Check your libretro frontend documentation for more information.
