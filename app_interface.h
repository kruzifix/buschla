#pragma once

#include "imgui/imgui.h"

struct AppState
{
    ImGuiContext* context;
    void* stateMemory;
    size_t stateMemorySize;
};
