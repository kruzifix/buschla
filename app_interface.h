#pragma once

#include "imgui/imgui.h"
#include "imgui/implot.h"

#include <limits.h>

struct AppState
{
    const char* exePath;

    ImGuiContext* imguiContext;
    ImPlotContext* implotContext;

    float time;
    float dt;

    void* stateMemory;
    size_t stateMemorySize;
};
