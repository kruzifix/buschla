#include "app_interface.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "imgui/imgui_internal.h"

#define SPLITTER_SIZE 10.f
#define SPLIT_MIN_CONTENT_SIZE 100.f

// NOTE: The memory for the state is automatically allocated.
// To ensure compatibility between States when hot-reloading,
// ONLY EVER add stuff to the end of this struct (it is allowed to grow).
// IF you need to re-arrange members, then you must restart the whole program!
// If stuff is added at the end, it is zero initialized!
typedef struct {
    float xSplit;
    float ySplitLeft;
    float ySplitRight;
} State;

// TODO: RIGHT CLICK => reset split!
static void splitter(const char* label, float& splitValue, bool horizontal) {
    ImVec2 windowSize = ImGui::GetWindowSize();

    ImVec2 size = horizontal ? ImVec2(SPLITTER_SIZE, windowSize.y) : ImVec2(windowSize.x, SPLITTER_SIZE);
    ImGui::Button(label, size);

    bool hovered = ImGui::IsItemHovered();
    bool active = ImGui::IsItemActive();
    if (hovered || active) {
        ImGui::SetMouseCursor(horizontal ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
    }

    if (active) {
        int axisIndex = horizontal ? 0 : 1;
        splitValue = ImClamp(splitValue + ImGui::GetIO().MouseDelta[axisIndex], SPLIT_MIN_CONTENT_SIZE, windowSize[axisIndex] - SPLIT_MIN_CONTENT_SIZE);
    }
}

static void gui(State* state) {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            // ShowExampleMenuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
            }
            if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
            } // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "Ctrl+X")) {
            }
            if (ImGui::MenuItem("Copy", "Ctrl+C")) {
            }
            if (ImGui::MenuItem("Paste", "Ctrl+V")) {
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    // ImGui::ShowStyleEditor();

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::GetStyle().WindowPadding = ImVec2(0, 0);

    bool open = true;
    if (ImGui::Begin("Fullscreen Window", &open, flags)) {
        ImVec2 windowSize = ImGui::GetWindowSize();
        // TODO: This does not seem to be working ...
        if (state->xSplit <= 0.f) {
            state->xSplit = windowSize.x - 300.f;
        }
        if (state->ySplitLeft <= 0.f) {
            state->ySplitLeft = windowSize.y * .5f;
        }
        if (state->ySplitRight <= 0.f) {
            state->ySplitRight = windowSize.y * .5f;
        }

#define STYLE_VAR(name, value)                \
    {                                         \
        ImGui::PushStyleVar((name), (value)); \
        ++styleVars;                          \
    }
#define STYLE_VAR2(name, x, y)                         \
    {                                                  \
        ImGui::PushStyleVar((name), ImVec2((x), (y))); \
        ++styleVars;                                   \
    }

        int styleVars = 0;
        STYLE_VAR2(ImGuiStyleVar_ItemSpacing, 0.f, 0.f);
        STYLE_VAR(ImGuiStyleVar_ChildBorderSize, 0.f);

        ImGui::BeginChild("region_left", ImVec2(state->xSplit, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_left_top", ImVec2(state->xSplit, state->ySplitLeft));
        // TODO: Inside the regions we do probably want ItemSpacing!!!
        ImGui::Text("Top Left");

        ImGui::EndChild();

        splitter("##region_left_splitter_v", state->ySplitLeft, false);

        ImGui::BeginChild("region_left_bot", ImVec2(state->xSplit, 0));
        ImGui::Text("Bot Left");
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::SameLine();
        splitter("##region_splitter_h", state->xSplit, true);

        ImGui::SameLine();

        ImGui::BeginChild("region_right", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_right_top", ImVec2(0, state->ySplitRight));
        ImGui::Text("Top Right");

        ImGui::EndChild();

        splitter("##region_right_splitter_v", state->ySplitRight, false);

        ImGui::BeginChild("region_right_bot", ImVec2(0, 0));
        ImGui::Text("Bot Right");
        ImGui::EndChild();

        ImGui::EndChild();
        ImGui::PopStyleVar(styleVars);
    }
    ImGui::End();
}

static State* getStateMemory(AppState* state) {
    size_t stateSize = sizeof(State);
    // this means we are right after startup
    if (state->stateMemory == NULL) {
        state->stateMemory = malloc(stateSize);
        state->stateMemorySize = stateSize;
        printf("[APP] No state memory, allocating %ld.\n", stateSize);
    }
    // this means we hot-reloading and added something to the State struct
    else if (stateSize > state->stateMemorySize) {
        printf("[APP] More state memory required, allocating %ld (was %ld).\n", stateSize, state->stateMemorySize);

        void* newMemory = malloc(stateSize);
        memset(newMemory, 0, stateSize);
        // copy over old contents
        memcpy(newMemory, state->stateMemory, state->stateMemorySize);

        free(state->stateMemory);
        state->stateMemory = newMemory;
        state->stateMemorySize = stateSize;
    }

    return (State*)state->stateMemory;
}

extern "C" void app_main(AppState* appState) {
    State* state = getStateMemory(appState);

    ImGui::SetCurrentContext(appState->context);

    gui(state);

    // ImGui::ShowStyleEditor();
}
