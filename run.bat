@echo off

rmdir /S /Q build

call "build_win64.bat"

call build\buschla.exe
