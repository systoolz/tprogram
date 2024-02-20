@echo off
rem GCC 3.2 (DEV-CPP) build file
tskill tprogram
if exist tprogram.exe del tprogram.exe
cls
gcc -mwindows -Dmain=_main -fno-exceptions -fno-rtti -mno-stack-arg-probe -fwritable-strings -s -Os -Wall -ansi -pedantic -nostdlib win32api.c gkiounit.c testunit.c tprogram.c -o tprogram.exe -l kernel32 -l user32 -l gdi32
