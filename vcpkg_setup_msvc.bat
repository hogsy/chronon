@echo off

if not exist vcpkg git clone https://github.com/Microsoft/vcpkg.git
if not exist vcpkg\vcpkg.exe call vcpkg\bootstrap-vcpkg.bat -disableMetrics

cd vcpkg

REM check it's all up-to-date
git pull
vcpkg.exe update

REM now install what we need
vcpkg\vcpkg.exe install sdl2:x64-windows-static sdl2:x86-windows

echo Done! Check above for any errors/warnings.