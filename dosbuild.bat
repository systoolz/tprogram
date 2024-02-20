@echo off
rem Turbo C 2.01 build file
tskill ntvdm
if exist DPROGRAM.EXE del DPROGRAM.EXE
cls
echo TC201\LIB\C0S.OBJ+>TURBOC.$LN
for %%a in (*.C) do (
  TC201\BIN\TCC.EXE -w -g1 -c -Z -O -ms -ITC201\INCLUDE %%a
  if errorlevel 1 goto :quit
  echo %%~na+>>TURBOC.$LN
)
echo TC201\LIB\CS.LIB>>TURBOC.$LN
echo DPROGRAM.EXE>>TURBOC.$LN
echo /c/x>>TURBOC.$LN
TC201\BIN\TLINK.EXE @TURBOC.$LN
:quit
del TURBOC.$LN
del *.OBJ
tskill ntvdm
