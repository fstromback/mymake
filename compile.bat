@echo off

echo Compiling mymake...

if exist build goto build
md build

:build

if exist bin goto compile
md bin

:compile

cl /nologo /EHsc textinc\*.cpp /Fobuild\ /Febin\textinc.exe
bin\textinc.exe bin\templates.h templates\project.txt templates\target.txt templates\global.txt
cl /nologo /O2 /EHsc src\*.cpp /Fobuild\ /Femymake.exe

