@echo off
@REM Build for Visual Studio compiler. Run your copy of vcvars64.bat or vcvarsall.bat to setup command-line compiler.

@REM TODO: Should we also do a unity build, so the build is as close to the linux build?
@REM  BUT, if we refactor to nob, then we can do that in there anyway.
set OUT_EXE=buschla
set INCLUDES=/I%SDL3_DIR%\include
set SOURCES=main.cpp util.cpp dynamic_array.cpp imgui/*.cpp 
set LIBS=/LIBPATH:%SDL3_DIR%\lib\x64 SDL3.lib shell32.lib

set OUT_DIR=build
mkdir %OUT_DIR%
cl /nologo /Zi /MD /utf-8 /DWINDOWS %INCLUDES% %SOURCES% /Fe%OUT_DIR%/%OUT_EXE%.exe /Fo%OUT_DIR%/ /link %LIBS% /subsystem:console

cp %SDL3_DIR%/lib/x64/SDL3.dll %OUT_DIR%
cp DroidSansMono.ttf %OUT_DIR%\font.ttf
