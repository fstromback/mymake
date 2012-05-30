@echo off
call "c:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" x86
"C:\shell\vs\mymake.exe" %*
