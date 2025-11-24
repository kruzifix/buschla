#pragma once

#include "imgui/imgui.h"

struct AppState
{
    ImGuiContext* context;
    float time;
    float dt;

    void* stateMemory;
    size_t stateMemorySize;
};
