# Makefile for compiling for Linux
# SDL3-devel package needs to be installed
# For Windows build see build_win64.bat

#CXX = g++
#CXX = clang++

EXE = buschla
APP_SO = app.so
IMGUI_DIR = imgui
BUILD_DIR = build

SOURCES = main.cpp
SOURCES += util.cpp
SOURCES += directory_watcher.cpp

SOURCES += $(IMGUI_DIR)/imgui.cpp
SOURCES += $(IMGUI_DIR)/imgui_demo.cpp
SOURCES += $(IMGUI_DIR)/imgui_draw.cpp
SOURCES += $(IMGUI_DIR)/imgui_tables.cpp
SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp

SOURCES += $(IMGUI_DIR)/imgui_impl_sdl3.cpp
SOURCES += $(IMGUI_DIR)/imgui_impl_sdlgpu3.cpp

OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o, $(basename $(notdir $(SOURCES)))))

APP_SOURCES = app.cpp
APP_OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o, $(basename $(notdir $(APP_SOURCES)))))

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR)
CXXFLAGS += -g -ggdb -Wall -Wformat# -pedantic
CXXFLAGS += `pkg-config sdl3 --cflags`
LIBS = -ldl `pkg-config sdl3 --libs`

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

# TODO: Is a unity build for the main app and the hot reloadable code better?

## Main executable

all: $(BUILD_DIR)/$(EXE) $(BUILD_DIR)/$(APP_SO)
	@echo Done building $(EXE) and $(APP_SO).

# Link executable
$(BUILD_DIR)/$(EXE): $(OBJS)
	$(CXX) -rdynamic -o $@ $^ $(CXXFLAGS) $(LIBS)

# Build source files to .o
$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Build imgui source files to .o
$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

## Dynamic library

app: $(BUILD_DIR)/$(APP_SO)
	@echo Done building $(APP_SO).

# Link dynamic library
$(BUILD_DIR)/$(APP_SO): $(APP_OBJS)
	$(CXX) -shared -o $@ $^ $(CXXFLAGS) $(LIBS)

$(APP_OBJS): $(APP_SOURCES)
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -fPIC -c $< -o $@

.PHONY: run
run: $(BUILD_DIR)/$(EXE) $(BUILD_DIR)/$(APP_SO)
	./$(BUILD_DIR)/$(EXE)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo Removed build directory \'$(BUILD_DIR)/\'
