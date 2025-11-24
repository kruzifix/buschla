#include "app_interface.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"
#include "dynamic_array.h"

#include "imgui/imgui_internal.h"

#define SPLITTER_SIZE 10.f
#define SPLIT_MIN_CONTENT_SIZE 100.f

typedef struct {
    char* txt;
    uint32_t len;
} StrView;

typedef struct {
    uint32_t lineNum;
    StrView str;
} LogLine;

DEFINE_DYNAMIC_ARRAY(LogLines, LogLine)
DEFINE_DYNAMIC_ARRAY(Chars, char)

// NOTE: The memory for the state is automatically allocated.
// To ensure compatibility between States when hot-reloading,
// ONLY EVER add stuff to the end of this struct (it is allowed to grow).
// IF you need to re-arrange members, then you must restart the whole program!
// If stuff is added at the end, it is zero initialized!
typedef struct {
    float xSplit;
    float ySplitLeft;
    float ySplitRight;

    const char* pendingFile;
    FILE* file;

    Chars textBuffer;
    LogLines logLines;

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

        // TODO: Create RAII objs to handle this nicely.
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

// Returns pointer to statically allocated buffer. Don't store this pointer!
// The contents will be modified the next time fetchLine is called.
// If EOF or an error occured while reading, NULL is returned instead.
// TODO: For now we cut off any line longer than 4K characters.
// Is that an ok assumption to make?  ¯\_(ツ)_/¯
static char fetchLineBuffer[4 * 1024];
static char* fetchLine(FILE* stream, uint32_t* lengthOut) {
    if (stream == NULL) {
        return NULL;
    }

    uint32_t length = 0;
    while (length < sizeof(fetchLineBuffer)) {
        char c;
        size_t count = fread(&c, 1, 1, stream);
        if (count != 1) {
            if (length == 0) {
                return NULL;
            }

            break;
        }

        if (c == '\n') {
            break;
        }

        // Ignore other non-printable characters
        // 0x7F is DEL
        if (ASCII_IS_CONTROL(c) || UTF8_IS_CONTINUATION(c)) {
            continue;
        }

        bool isAscii = c >= 0 && c < 0x80;
        bool isCodePointStart = UTF8_IS_START(c);

        bool addToBuffer = isAscii;
        if (isCodePointStart) {
            c = '?';
            addToBuffer = true;
        }

        if (addToBuffer) {
            fetchLineBuffer[length] = c;
            ++length;
        }
    }

    *lengthOut = length;
    return fetchLineBuffer;
}

static void loadLogFile(State* state) {
    // LOG FILE LOADING
    // NOTE: for text pasted from clipboard: paste to file and then use the same interface for loading log from file!
    // TODO: what if the file is modified?
    // => either file was overwritten
    // => or stuff was appended
    // we have to detect which happened, and then act accordingly!

    if (state->file == NULL && state->pendingFile != NULL) {
        state->file = fopen(state->pendingFile, "r");
        if (state->file == NULL) {
            perror("fopen state->file");
            return;
        }

        printf("[APP] started fetching file '%s'\n", state->pendingFile);
        state->pendingFile = NULL;

        // if (state->logLines.items == NULL) {
        //     da_reserve(&state->logLines, 512);
        // }
    }

    if (state->file != NULL) {
        uint32_t length = 0;
        char* line = NULL;
        float elapsedTime = 0.f;

        int lines = 0;
        while (elapsedTime < 5.f) {
            TIME_SCOPE(fetchLineTimer) {
                line = fetchLine(state->file, &length);
                ++lines;
            }

            elapsedTime += fetchLineTimer.elapsedMs;
            if (line == NULL) {
                break;
            }
        }

        printf("[APP] fetched %d lines in %.3fms\n", lines, elapsedTime);

        if (line == NULL) {
            int ret = fclose(state->file);
            if (ret != 0) {
                perror("fclose state->file");
            }

            state->file = NULL;
            printf("[APP] finished fetching file\n");
        }
    }
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

char filePath[PATH_MAX];

extern "C" void app_main(AppState* appState) {
    State* state = NULL;

    TIME_SCOPE(guiTimer) {
        state = getStateMemory(appState);

        ImGui::SetCurrentContext(appState->context);

        gui(state);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        concatPaths(filePath, appState->exePath, "../log.txt");
        state->pendingFile = filePath;
    }

    loadLogFile(state);

    // ImGui::ShowStyleEditor();
}
