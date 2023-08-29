# Chronon

[![Build (Windows GCC x64)](https://github.com/hogsy/hosae/actions/workflows/build-windows-gcc_x64.yml/badge.svg)](https://github.com/hogsy/hosae/actions/workflows/build-windows-gcc_x64.yml)
[![Build (Linux GCC x64)](https://github.com/hogsy/hosae/actions/workflows/build-linux-gcc_x64.yml/badge.svg)](https://github.com/hogsy/hosae/actions/workflows/build-linux-gcc_x64.yml)

## What is this?

This is an open-source reimplementation of Anachronox, built on top of the Quake 2 engine (which funnily enough is the same engine Anachronox was developed upon).

It's important to note that this project is operating to reimplement the game through publicly available documentation and observation, rather than dissassembly, which should hopefully be fine from a legal standpoint.

## Project Goals

- Reimplement Anachronox (duh)
- Support for multiple platforms; linux, android and more
- Make the game easier to modify
- Improved visuals

## Immediate Roadmap

- [ ] Modernise renderer w/ modern OpenGL (i.e. 3.3)
- [ ] Finalise MD2 loader
  - [ ] Multiple skin support
  - [ ] Vertex key morphing (for facial animation)
  - [ ] MDA specification - ensure all features are supported
- [ ] Virtual machine for APE scripts

## Features

[![Screenshot](preview/thumb_01.png)](preview/01.webp)
[![Screenshot](preview/thumb_02.png)](preview/02.webp)
[![Screenshot](preview/thumb_03.png)](preview/03.webp)
[![Screenshot](preview/thumb_04.png)](preview/04.webp)
[![Screenshot](preview/thumb_06.png)](preview/06.webp)

It's still in a very early stage of development, so nothing is playable yet.
Below is a list of what's been done thus far.

- Loading from Anachronox's _DAT_ packages
- Engine can initialise itself from the Anachronox game directory
- Some preliminary work on loading Anachronox's models
- Supports the various texture formats Anachronox uses (e.g. PNG, BMP and TGA)
- Respects custom texture surface flags, inc. alpha effects for textures
- Extended limits, which is necessary for some maps to load

### Extras

These are additional enhancements introduced here that the original game didn't necessarily feature.

- Can target x86, x64 and ARM64 architectures
- Support for both Microsoft Windows and Linux
- Overbrights via `r_overbrights` (just be wary Anachronox's art was not designed for it!)
- Higher-resolution texture replacements can be loaded in
    - Replacement textures need to be placed under a `hd` directory located under `anoxdata`
    - Can be enabled/disabled via `hd_override` cvar
- Replaced QGL with GLEW
- Code is compiled as C++, as opposed to C

## Wow, neat! How can I help?

If you have experience with either C/C++, have a passion for programming and familiarity with the Anachronox game, then feel free to get in touch via our [Discord](https://discord.gg/EdmwgVk) server in the `#anachronox` channel.

Alternatively, feel free to ping me an email at [hogsy@oldtimes-software.com](mailto:hogsy@oldtimes-software.com).

## Building

**(these instructions aren't necessarily up-to-date)**

### Windows

#### Visual Studio

1. Use `vcpkg_setup_msvc.bat`; this will fetch vcpkg and install the dependencies
2. Now use `cmake_generate_vs2022-x64.bat` which will generate the Visual Studio solution for you under a new `build` directory

#### MinGW-w64 

You can download MinGW-w64 from [here](https://www.mingw-w64.org/) and of course will require
CMake as well which can be found [here](https://cmake.org/).

For now a copy of SDL2 is retained in the repo specifically for compiling against MinGW-w64, as
the one included in *vcpkg* currently doesn't compile.

### macOS w/ Apple Silicon

No plans to support "legacy" x86/x64 based devices right now unless someone else steps in to do it.
macOS support is generally going to be at the bottom of my priority list for supported platforms.

1. If you don't have vcpkg installed already, use `vcpkg_setup_apple.sh`; this will fetch vcpkg and install the dependencies
2. Use CMake as usual, but pass `-DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake` as an argument so it can find packages provided by vcpkg

## Resources

- [Anachrodox](https://anachrodox.talonbrave.info/)
