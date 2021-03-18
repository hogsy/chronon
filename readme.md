# HOSAE<br>hogsy's Open-Source Anachronox Engine

![Screenshot](preview/hosae_IK5L4eTmXq.png)

## What is this?

This is an open-source reimplementation of Anachronox, built on top of the
Quake 2 engine (which funnily enough is the same engine Anachronox was
developed upon).

The project has the following goals, somewhat in order of priority.

- Reimplement Anachronox (duh)
- Support for multiple platforms; linux, android and more
- Make the game easier to modify (i.e. support for larger texture sizes)
- Improved visuals

It's still in a very early stage of development so nothing is playable yet.
Below is a list of what's been done thus far.

- Support for loading from Anachronox's data packages; engine can initialize itself from the Anachronox game directory
- Some preliminary work on loading Anachronox's models
- Support for the various texture formats Anachronox uses (e.g. PNG, BMP and TGA)
- Replaced QGL with GLEW
- Code is compiled as C++, as opposed to C
- Fixed a number of potential buffer overflows in the Q2 engine

## Wow, neat! How can I help?

If you have experience with either C/C++, have a passion for programming and familiarity with the Anachronox game, then feel free to get in touch via our [Discord](https://discord.gg/EdmwgVk) server in the dedicated `#hosae` channel.

Alternatively, feel free to ping me an email at [hogsy@oldtimes-software.com](mailto:hogsy@oldtimes-software.com).

## Why 'HOSAE'?

I couldn't come up with a better name for the project at the time.
Hosae is pretty much a placeholder name for the time being anyway 
until the project is a bit further along, and I thought it fit well with the humour of the game.
