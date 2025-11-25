#pragma once

#include "imgui/imgui.h"

#include <limits.h>

struct AppState
{
    const char* exePath;

    ImGuiContext* context;
    float time;
    float dt;

    void* stateMemory;
    size_t stateMemorySize;
};
