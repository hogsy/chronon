#!/bin/bash

VCDIR="./vcpkg/"
VCEXE="./vcpkg/vcpkg"

if [ ! -d "$VCDIR" ]; then
    git clone https://github.com/Microsoft/vcpkg.git
fi
if [ ! -f "$VCEXE" ]; then
    vcpkg/bootstrap-vcpkg.sh -disableMetrics
fi

# now install what we need
vcpkg/vcpkg install sdl2:x64-linux
