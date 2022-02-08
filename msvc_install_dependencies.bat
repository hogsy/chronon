@echo off

if not exist vcpkg git clone https://github.com/Microsoft/vcpkg.git
if not exist vcpkg\vcpkg.exe call vcpkg\bootstrap-vcpkg.bat -disableMetrics

REM now install what we need
vcpkg\vcpkg.exe install sdl2:x64-windows