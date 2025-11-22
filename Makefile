# Makefile for compiling for Linux
# SDL3-devel package needs to be installed
# For Windows build see build_win64.bat

COMPILER = g++
#COMPILER = clang++

CFLAGS =  -std=c++11
CFLAGS += -ggdb
CFLAGS += -Wall
#CFLAGS += -Wformat
CFLAGS += `pkg-config sdl3 --cflags`

LDFLAGS =  -ldl
LDFLAGS += `pkg-config sdl3 --libs`

COMPILE = $(COMPILER) $(CFLAGS) -c
LINK = $(COMPILER) $(LDFLAGS)

## ----------------------------- ##

BUILD_DIR = ./build
IMGUI_DIR = ./imgui

APP_LIB = $(BUILD_DIR)/app.so

## ----------------------------- ##

EXE = $(BUILD_DIR)/buschla

.PHONY: all
all: $(EXE) $(APP_LIB)
	@echo Finished building BUSCHLA!

.PHONY: app
app: $(APP_LIB)
	@echo Finished building app library!

## ----------------------------- ##

# all *.cpp files in imgui folder
IMGUI_SRC_FILES = $(wildcard $(IMGUI_DIR)/*.cpp)
IMGUI_SRC_UNITY = $(BUILD_DIR)/unity_imgui.cpp
IMGUI_OBJ = $(BUILD_DIR)/imgui.o

# build imgui .o file
$(IMGUI_OBJ): $(IMGUI_SRC_UNITY)
	$(COMPILE) -fPIC -o $(IMGUI_OBJ) $(IMGUI_SRC_UNITY)

# compose imgui unity source file
$(IMGUI_SRC_UNITY): $(IMGUI_SRC_FILES) $(BUILD_DIR)
	@echo -e $(IMGUI_SRC_FILES:%='\n#include "../%"') > $(IMGUI_SRC_UNITY)

## ----------------------------- ##

EXE_SRC =  util
EXE_SRC += directory_watcher
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
$(EXE_SRC_UNITY): $(EXE_SRC_FILES) $(BUILD_DIR)
	@echo -e $(EXE_SRC_INCLUDES) > $(EXE_SRC_UNITY)

## ----------------------------- ##

APP_SRC = app

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
$(APP_SRC_UNITY): $(APP_SRC_FILES) $(BUILD_DIR)
	@echo -e $(APP_SRC_INCLUDES) > $(APP_SRC_UNITY)

## ----------------------------- ##

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

.PHONY: run
run: $(EXE) $(APP_LIB)
	./$(EXE)

.PHONY: clean
clean:
	@rm -rf $(BUILD_DIR)
	@echo Removed build directory \'$(BUILD_DIR)/\'
