@echo off
rem dual-executable build file
call dosbuild.bat
call winbuild.bat
buildexe\dexem.exe /r tprogram.exe DPROGRAM.EXE
if exist DPROGRAM.EXE del DPROGRAM.EXE
