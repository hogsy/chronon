@echo off
mkdir build
cmake -G "Visual Studio 17 2022" -A x64 -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake
pause