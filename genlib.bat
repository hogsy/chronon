rem https://stackoverflow.com/a/39916789
rem Needs to be used from the Visual Studio Developer Command Prompt

@echo off

setlocal enabledelayedexpansion
for /f "tokens=1-4" %%1 in ('dumpbin /exports %1') do (
    set /a ordinal=%%1 2>nul
    set /a hint=0x%%2 2>nul
    set /a rva=0x%%3 2>nul
    if !ordinal! equ %%1 if !hint! equ 0x%%2 if !rva! equ 0x%%3 set exports=!exports! /export:%%4
)

for /f %%i in ("%1") do set dllpath=%%~dpni
start lib /out:%dllpath%.lib /machine:x86 /def: %exports%