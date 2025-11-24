#pragma once

#include "imgui/imgui.h"

#include <limits.h>

struct AppState
{
    char exePath[PATH_MAX];

    ImGuiContext* context;
    float time;
    float dt;

    void* stateMemory;
    size_t stateMemorySize;
};
