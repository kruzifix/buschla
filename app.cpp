#include "app_interface.h"

#define SPLITTER_SIZE 10.f
#define SPLIT_MIN_CONTENT_SIZE 100.f

static inline float clampf(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

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
        splitValue = clampf(splitValue + ImGui::GetIO().MouseDelta[axisIndex], SPLIT_MIN_CONTENT_SIZE, windowSize[axisIndex] - SPLIT_MIN_CONTENT_SIZE);
    }
}

static void gui() {
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
        ImGuiWindowFlags_NoBackground;

    // We demonstrate using the full viewport area or the work area (without menu-bars, task-bars etc.)
    // Based on your use case you may want one or the other.
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);

    ImGui::GetStyle().WindowPadding = ImVec2(0, 0);

    bool open = true;
    if (ImGui::Begin("Fullscreen Window", &open, flags)) {
        static float xSplit = -1.f;
        static float ySplitLeft = -1.f;
        static float ySplitRight = -1.f;

        ImVec2 windowSize = ImGui::GetWindowSize();
        if (xSplit < 0.f) {
            xSplit = windowSize.x - 300.f;
        }
        if (ySplitLeft < 0.f) {
            ySplitLeft = windowSize.y * .5f;
        }
        if (ySplitRight < 0.f) {
            ySplitRight = windowSize.y * .5f;
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

        ImGui::BeginChild("region_left", ImVec2(xSplit, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_left_top", ImVec2(xSplit, ySplitLeft));
        // TODO: Inside the regions we do probably want ItemSpacing!!!
        ImGui::Text("Top Left");

        ImGui::EndChild();

        splitter("##region_left_splitter_v", ySplitLeft, false);

        ImGui::BeginChild("region_left_bot", ImVec2(xSplit, 0));
        ImGui::Text("Bot Left");
        ImGui::EndChild();

        ImGui::EndChild();

        ImGui::SameLine();
        splitter("##region_splitter_h", xSplit, true);

        ImGui::SameLine();

        ImGui::BeginChild("region_right", ImVec2(0.f, 0.f), ImGuiChildFlags_Borders);

        ImGui::BeginChild("region_right_top", ImVec2(0, ySplitRight));
        ImGui::Text("Top Right");
        ImGui::EndChild();

        splitter("##region_right_splitter_v", ySplitRight, false);

        ImGui::BeginChild("region_right_bot", ImVec2(0, 0));
        ImGui::Text("Bot Right");
        ImGui::EndChild();

        ImGui::EndChild();
        ImGui::PopStyleVar(styleVars);
    }
    ImGui::End();

    // ImGui::ShowDemoWindow(&open);
}

extern "C" void app_main(AppState& state)
{
    ImGui::SetCurrentContext(state.context);

    gui();
}
