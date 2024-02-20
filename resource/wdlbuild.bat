@echo off
cd script
tcompile.exe
move *.BIN ..\
copy DOS8X8RU.FNT ..\DOS8X8RU.BIN
cd ..
cd images
compress.exe -r *.BMP
move *.BM_ ..\
cd ..
packer\wdlzpack.exe TESTINFO.BIN MESSAGES.BIN TESTTEXT.BIN DOS8X8RU.BIN *.BM_
del *.BM_
del *.BIN
