# Makefile for compiling for Linux
# SDL3-devel package needs to be installed
# For Windows build see build_win64.bat

#CXX = g++
#CXX = clang++

EXE = buschla
IMGUI_DIR = imgui
BUILD_DIR = build

SOURCES = main.cpp

SOURCES += $(IMGUI_DIR)/imgui.cpp
SOURCES += $(IMGUI_DIR)/imgui_demo.cpp
SOURCES += $(IMGUI_DIR)/imgui_draw.cpp
SOURCES += $(IMGUI_DIR)/imgui_tables.cpp
SOURCES += $(IMGUI_DIR)/imgui_widgets.cpp

SOURCES += $(IMGUI_DIR)/imgui_impl_sdl3.cpp
SOURCES += $(IMGUI_DIR)/imgui_impl_sdlgpu3.cpp

OBJS = $(addprefix $(BUILD_DIR)/,$(addsuffix .o, $(basename $(notdir $(SOURCES)))))

CXXFLAGS = -std=c++11 -I$(IMGUI_DIR)
CXXFLAGS += -g -Wall -Wformat -pedantic
LIBS =

UNAME_S := $(shell uname -s)
## TODO: Test if this is required?
ifeq ($(UNAME_S), Linux) #LINUX
	ECHO_MESSAGE = "Linux"
	LIBS += -ldl `pkg-config sdl3 --libs`

	CXXFLAGS += `pkg-config sdl3 --cflags`
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES
##---------------------------------------------------------------------

all: $(BUILD_DIR)/$(EXE)
	@echo Build complete for $(ECHO_MESSAGE)
	$(BUILD_DIR)/$(EXE)

$(BUILD_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(IMGUI_DIR)/%.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo Removed build directory \'$(BUILD_DIR)/\'
