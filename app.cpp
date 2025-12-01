#include "app_interface.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "dynamic_array.h"

#include "buschla_file.h"

#include "imgui/imgui_internal.h"
#include "imgui/implot.h"

#define SPLITTER_SIZE 10.f
#define SPLIT_MIN_CONTENT_SIZE 100.f

// NOTE: The memory for the state is automatically allocated.
// To ensure compatibility between States when hot-reloading,
// ONLY EVER add stuff to the end of this struct (it is allowed to grow).
// IF you need to re-arrange members, then you must restart the whole program!
// If stuff is added at the end, it is zero initialized!
typedef struct {
    // Stores width of right(!) region.
    float xSplit;

    float ySplitLeft;
    float ySplitRight;

    BuschlaFile* buschlaFile;

} State;

// TODO: RIGHT CLICK => reset split!
static void splitter(const char* label, float* splitValue, bool horizontal) {
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
        *splitValue = ImClamp(*splitValue + ImGui::GetIO().MouseDelta[axisIndex], SPLIT_MIN_CONTENT_SIZE, windowSize[axisIndex] - SPLIT_MIN_CONTENT_SIZE);
    }
}

struct ScopedStyleVar {
    ScopedStyleVar(ImGuiStyleVar idx, const float value) {
        ImGui::PushStyleVar(idx, value);
    }

    ScopedStyleVar(ImGuiStyleVar idx, const ImVec2& value) {
        ImGui::PushStyleVar(idx, value);
    }

    ScopedStyleVar(ImGuiStyleVar idx, float x, float y) {
        ImGui::PushStyleVar(idx, ImVec2(x, y));
    }

    ~ScopedStyleVar() {
        ImGui::PopStyleVar();
    }
};

#define CAT_(a, b) a ## b
#define CAT(a, b) CAT_(a, b)
#define UNIQUE_NAME(prefix) CAT(prefix, __LINE__)

#define SCOPE_STYLE(name, value) ScopedStyleVar UNIQUE_NAME(_scopeStyleVar_)((name), (value))
#define SCOPE_STYLE2(name, x, y) ScopedStyleVar UNIQUE_NAME(_scopeStyleVar_)((name), (x), (y))

char filePath[PATH_MAX];

static void gui(AppState* appState, State* state) {
    if (ImGui::BeginMainMenuBar()) {
        // if (ImGui::BeginMenu("File")) {
        //     // ShowExampleMenuFile();
        //     ImGui::EndMenu();
        // }
        // if (ImGui::BeginMenu("Edit")) {
        //     if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
        //     }
        //     if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false)) {
        //     } // Disabled item
        //     ImGui::Separator();
        //     if (ImGui::MenuItem("Cut", "Ctrl+X")) {
        //     }
        //     if (ImGui::MenuItem("Copy", "Ctrl+C")) {
        //     }
        //     if (ImGui::MenuItem("Paste", "Ctrl+V")) {
        //     }
        //     ImGui::EndMenu();
        // }
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

        // TODO: Also handle the case where the split value is bigger than the window size!
        // (can happen if the window is resized to be smaller)

        // TODO: When window is resized, we kinda want the right section to stay the same size, right?
        // That means we need to store the width of the right section, instead of the left.
        if (state->xSplit <= 0.f) {
            state->xSplit = 300.f;
        }
        // TODO: Do we want the bottom left section to always stay the same size?
        // Then this would need to be inverted just like we do for the xSplit!
        if (state->ySplitLeft <= 0.f) {
            state->ySplitLeft = windowSize.y * .5f;
        }
        if (state->ySplitRight <= 0.f) {
            state->ySplitRight = windowSize.y * .5f;
        }

        SCOPE_STYLE2(ImGuiStyleVar_ItemSpacing, 0.f, 0.f);
        SCOPE_STYLE(ImGuiStyleVar_ChildBorderSize, 0.f);

        float widthLeft = windowSize.x - state->xSplit;
        ImGui::BeginChild("region_left", ImVec2(widthLeft, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_left_top", ImVec2(widthLeft, state->ySplitLeft),
            0, ImGuiWindowFlags_HorizontalScrollbar);
        {
            if (state->buschlaFile != NULL) {
                uint32_t logLineCount = state->buschlaFile->header->logLineCount;
                for (uint32_t i = 0; i < logLineCount; ++i) {
                    ImGui::PushID(i);

                    LogLine* logLine = state->buschlaFile->logLines + i;
                    // TODO: determine width of line num with line count!
                    ImGui::Text("%6d", logLine->lineNum);
                    ImGui::SameLine(0.f, 4.f);
                    const char* txt = logLine->str.txt;
                    ImGui::TextEx(txt, txt + logLine->str.len);

                    ImGui::PopID();
                }
            }
        }
        ImGui::EndChild();

        splitter("##region_left_splitter_v", &state->ySplitLeft, false);

        ImGui::BeginChild("region_left_bot", ImVec2(widthLeft, 0));
        {
            static float xs1[1001], ys1[1001];
            for (int i = 0; i < 1001; ++i) {
                xs1[i] = i * 0.001f;
                ys1[i] = 0.5f + 0.5f * sinf(50 * (xs1[i] + (float)ImGui::GetTime() / 10));
            }
            static double xs2[20], ys2[20];
            for (int i = 0; i < 20; ++i) {
                xs2[i] = i * 1 / 19.0f;
                ys2[i] = xs2[i] * xs2[i];
            }

            ImPlotFlags flags =
                ImPlotFlags_NoTitle |
                //ImPlotFlags_NoLegend |
                ImPlotFlags_NoMouseText |
                ImPlotFlags_NoInputs |
                ImPlotFlags_NoMenus |
                ImPlotFlags_NoBoxSelect |
                ImPlotFlags_NoFrame |
                //ImPlotFlags_Equal |
                ImPlotFlags_Crosshairs;

            // TODO: If we have other widgets, this doesnt work!
            ImVec2 regionSize = ImGui::GetWindowSize();
            if (ImPlot::BeginPlot("Test Plot", ImVec2(regionSize.x, regionSize.y), flags)) {
                ImPlot::SetupAxes("x", "y");
                ImPlot::PlotLine("f(x)", xs1, ys1, 1001);
                ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle);
                // TODO: Checkout what Stride / Striding can do for us!!!
                ImPlot::PlotLine("g(x)", xs2, ys2, 20, ImPlotLineFlags_Segments);
                ImPlot::EndPlot();
            }
        }
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::SameLine();
        splitter("##region_splitter_h", &widthLeft, true);
        state->xSplit = windowSize.x - widthLeft;

        ImGui::SameLine();

        ImGui::BeginChild("region_right", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_right_top", ImVec2(0, state->ySplitRight));
        {
            SCOPE_STYLE2(ImGuiStyleVar_ItemSpacing, 5.f, 5.f);

            ImGui::Text("Top Right");

        }
        ImGui::EndChild();

        splitter("##region_right_splitter_v", &state->ySplitRight, false);

        ImGui::BeginChild("region_right_bot", ImVec2(0, 0));
        {
            ImGui::Text("Bot Right");
        }
        ImGui::EndChild();

        ImGui::EndChild();
    }
    ImGui::End();
}

static State* getStateMemory(AppState* state) {
    size_t stateSize = sizeof(State);
    // this means we are right after startup
    if (state->stateMemory == NULL) {
        state->stateMemory = malloc(stateSize);
        memset(state->stateMemory, 0, stateSize);
        state->stateMemorySize = stateSize;
        printf("[APP] No state memory, allocating %ld bytes.\n", stateSize);
    }
    // this means we hot-reloading and added something to the State struct
    else if (stateSize > state->stateMemorySize) {
        printf("[APP] More state memory required, allocating %ld bytes (was %ld bytes).\n", stateSize, state->stateMemorySize);

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
    State* state = NULL;

    TIME_SCOPE(guiTimer) {
        state = getStateMemory(appState);

        ImGui::SetCurrentContext(appState->imguiContext);
        ImPlot::SetCurrentContext(appState->implotContext);

        gui(appState, state);
    }

    if (state->buschlaFile == NULL) {
        TIME_SCOPE(loadBuschlaFileTimer) {
            state->buschlaFile = tryLoadBuschlaFile("out.buschla");
        }
        printf("Load buschlaFile took %.3fms\n", loadBuschlaFileTimer.elapsedMs);
    }

    // ImGui::ShowStyleEditor();
    // ImPlot::ShowDemoWindow();
}
