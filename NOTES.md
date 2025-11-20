# ------------------
# BUSCHLA
# Log Analyzer
# ------------------


#### Rebuild shader for imgui
The shader for imgui is embedded as SPIR-V in the source code.
If we want to change the shader, see **imgui/backends/sdlgpu3** for instructions.

#### Makefile reference
https://makefiletutorial.com/

## How to build on LINUX
- Install SDL3-devel (dnf install SDL3-devel)
- Run make

## How to build on WINDOWS
- Download SDL3 release https://github.com/libsdl-org/SDL/releases
- Add env variable SDL3_DIR and have it point to extracted folder
- Open "x64 Native Tools Command Prompt for VS 2022"
- Run build.bat


## Links
### ImGUI
https://github.com/ocornut/imgui
=> do we want to use C++ so we get nice operator overloading for vector stuff?
==> and maybe containers? ¯\_(ツ)_/¯

there do exist C APIs for imgui ...

https://github.com/epezent/implot

https://github.com/ocornut/imgui/wiki/Useful-Extensions

### TOML
https://github.com/toml-lang/toml/wiki

