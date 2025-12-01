# Makefile for compiling for Linux
# SDL3-devel package needs to be installed
# For Windows build see build_win64.bat

# Disable implicit rules so canonical targets will work.
.SUFFIXES:
# Disable VCS-based implicit rules.
% : %,v
% : RCS/%
% : RCS/%,v
% : SCCS/s.%
% : s.%

# TODO: Refactor this horrible mess with nob.h!!!!

COMPILER = g++
#COMPILER = clang++

CFLAGS =  -std=c++11
CFLAGS += -ggdb
#CFLAGS += -O3
CFLAGS += -Wall
CFLAGS += -DLINUX
#CFLAGS += -Wformat
CFLAGS += `pkg-config sdl3 --cflags`

LDFLAGS =  -ldl
LDFLAGS += `pkg-config sdl3 --libs`

COMPILE = $(COMPILER) $(CFLAGS) -c
LINK = $(COMPILER) $(LDFLAGS)

## ----------------------------- ##

BUILD_DIR = ./build
IMGUI_DIR = ./imgui

EXE = $(BUILD_DIR)/buschla
APP_LIB = $(BUILD_DIR)/app.so
PARSER_EXE = $(BUILD_DIR)/buschla-parser

## ----------------------------- ##

.PHONY: all
all: $(EXE) $(APP_LIB) $(PARSER_EXE)
	cp DroidSansMono.ttf $(BUILD_DIR)/font.ttf
	@printf '\033[32;1mFinished building BUSCHLA!\033[0m\n'

.PHONY: app
app: $(APP_LIB)
	@printf '\033[32;1mFinished building BUSCHLA app lib!\033[0m\n'

.PHONY: parser
parser: $(PARSER_EXE)
	@printf '\033[32;1mFinished building BUSCHLA parser!\033[0m\n'

## ----------------------------- ##

# all *.cpp files in imgui folder
IMGUI_SRC_FILES = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_SRC_UNITY = $(BUILD_DIR)/unity_imgui.cpp
IMGUI_OBJ = $(BUILD_DIR)/imgui.o

# build imgui .o file
$(IMGUI_OBJ): $(IMGUI_SRC_UNITY)
	$(COMPILE) -fPIC -o $(IMGUI_OBJ) $(IMGUI_SRC_UNITY)

# compose imgui unity source file
$(IMGUI_SRC_UNITY): $(IMGUI_SRC_FILES) | $(BUILD_DIR)
	@echo -e $(IMGUI_SRC_FILES:%='\n#include "../%"') > $(IMGUI_SRC_UNITY)

## ----------------------------- ##

EXE_SRC =  util
EXE_SRC += directory_watcher
EXE_SRC += dynamic_array
EXE_SRC += main

EXE_SRC_UNITY = $(BUILD_DIR)/unity_buschla.cpp
EXE_SRC_FILES = $(EXE_SRC:=.cpp)
EXE_SRC_INCLUDES = $(EXE_SRC_FILES:%='\n#include "../%"')
EXE_OBJ = $(EXE).o

# link exe
$(EXE): $(EXE_OBJ) $(IMGUI_OBJ)
	$(LINK) -o $(EXE) $(IMGUI_OBJ) $(EXE_OBJ)

# build exe .o file
$(EXE_OBJ): $(EXE_SRC_UNITY)
	$(COMPILE) -o $(EXE_OBJ) $(EXE_SRC_UNITY)

# compose exe unity source file
$(EXE_SRC_UNITY): $(EXE_SRC_FILES) | $(BUILD_DIR)
	@echo -e $(EXE_SRC_INCLUDES) > $(EXE_SRC_UNITY)

## ----------------------------- ##

APP_SRC = util
APP_SRC += dynamic_array
APP_SRC += buschla_file
APP_SRC += app

APP_SRC_UNITY = $(BUILD_DIR)/unity_app.cpp
APP_SRC_FILES = $(APP_SRC:=.cpp)
APP_SRC_INCLUDES = $(APP_SRC_FILES:%='\n#include "../%"')
APP_OBJ = $(APP_LIB).o

# link app
$(APP_LIB): $(APP_OBJ) $(IMGUI_OBJ)
	$(LINK) -shared -o $(APP_LIB) $(IMGUI_OBJ) $(APP_OBJ)

# build app .o file
$(APP_OBJ): $(APP_SRC_UNITY)
	$(COMPILE) -shared -fPIC -o $(APP_OBJ) $(APP_SRC_UNITY)

# compose app unity source file
$(APP_SRC_UNITY): $(APP_SRC_FILES) | $(BUILD_DIR)
	@echo -e $(APP_SRC_INCLUDES) > $(APP_SRC_UNITY)

## ----------------------------- ##

PARSER_SRC += util
#PARSER_SRC += directory_watcher
PARSER_SRC += dynamic_array
PARSER_SRC += parser

PARSER_SRC_UNITY = $(BUILD_DIR)/unity_pars.cpp
PARSER_SRC_FILES = $(PARSER_SRC:=.cpp)
PARSER_SRC_INCLUDES = $(PARSER_SRC_FILES:%='\n#include "../%"')
PARSER_OBJ = $(PARSER_EXE).o

# link parser exe
$(PARSER_EXE): $(PARSER_OBJ)
	$(LINK) -o $(PARSER_EXE) $(PARSER_OBJ)

# build parser exe .o file
$(PARSER_OBJ): $(PARSER_SRC_UNITY)
	$(COMPILE) -o $(PARSER_OBJ) $(PARSER_SRC_UNITY)

# compose parser exe unity source file
$(PARSER_SRC_UNITY): $(PARSER_SRC_FILES) | $(BUILD_DIR)
	@echo -e $(PARSER_SRC_INCLUDES) > $(PARSER_SRC_UNITY)

## ----------------------------- ##

$(BUILD_DIR):
	mkdir -p $@

.PHONY: run
run: all
	- killall buschla
	$(EXE)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo Removed build directory \'$(BUILD_DIR)/\'
