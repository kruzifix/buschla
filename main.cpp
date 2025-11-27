// Skeleton taken from imgui sdl3_sdlgpu3 example

#ifdef WINDOWS
#define PATH_MAX 4096
#else
#define ENABLE_HOT_RELOADING
#endif

#ifdef ENABLE_HOT_RELOADING
#include <dlfcn.h>
#include <sys/sendfile.h>
#include <unistd.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>

#include "app_interface.h"
#include "directory_watcher.h"
#include "dynamic_array.h"
#include "util.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_sdlgpu3.h"

#ifndef ENABLE_HOT_RELOADING
#include "app.cpp"
#endif

// TODO: How can we also build this to statically link the app layer?
// because we need the code hot reloading really only for dev on linux
typedef void (app_main_func)(AppState* state);

static SDL_FColor clear_color{ .1f, .1f, .1f, 1.f };

#ifdef ENABLE_HOT_RELOADING
typedef struct {
    void* handle;
    app_main_func* app_main;
} AppLibraryState;

// Returns false on error, check dlerror() for reason.
static bool tryLoadAppLibrary(const char* path, AppLibraryState* state) {
    void* handle = dlopen(path, RTLD_NOW);
    if (handle == NULL) {
        return false;
    }

    app_main_func* app_main = (app_main_func*)dlsym(handle, "app_main");
    if (app_main == NULL) {
        dlclose(handle);
        return false;
    }

    state->handle = handle;
    state->app_main = app_main;
    return true;
}

static void unloadAppLibrary(AppLibraryState* state) {
    if (state->handle != NULL) {
        dlclose(state->handle);

        state->handle = NULL;
        state->app_main = NULL;
    }
}

typedef struct {
    bool targetState;
    float t;
    float timeVisible;
    bool autoClose;

    bool success;
    // NOTE: this points to a strdup-ed string and is freed on re-trigger
    char* msg;
} HotReloadStatusWindow;

HotReloadStatusWindow hotReloadStatusWindow;

static inline float easeOutBack(float x) {
    static float c1 = 1.70158;
    static float c3 = c1 + 1;

    return 1.f + c3 * powf(x - 1.f, 3.f) + c1 * powf(x - 1.f, 2.f);
}

static void triggerHotReloadStatus(bool success, const char* msg) {
    hotReloadStatusWindow.targetState = true;
    hotReloadStatusWindow.t = 0.f;
    hotReloadStatusWindow.timeVisible = 0.f;

    hotReloadStatusWindow.success = success;
    hotReloadStatusWindow.autoClose = success;
    if (hotReloadStatusWindow.msg != NULL) {
        free(hotReloadStatusWindow.msg);
    }
    hotReloadStatusWindow.msg = strdup(msg);
}

static void drawHotReloadStatusWindow(AppState* state) {
    // ImGui::SetNextWindowPos(ImVec2(100, 100));
    // ImGui::Begin("asdlfkjwlekjf");
    // ImGui::Text("targetState: %d", hotReloadStatusWindow.targetState);
    // ImGui::Text("t: %.3f", hotReloadStatusWindow.t);
    // ImGui::Text("timeVisible: %.3f", hotReloadStatusWindow.timeVisible);
    // ImGui::Text("success: %d", hotReloadStatusWindow.success);
    // ImGui::Text("msg: %p", hotReloadStatusWindow.msg);
    // ImGui::End();

    const float animationDuration = .6f;
    const float fullyEasedInThreshold = .95f;

    if (hotReloadStatusWindow.t < .05f &&
        !hotReloadStatusWindow.targetState) {
        return;
    }

    float delta = state->dt / animationDuration * (hotReloadStatusWindow.targetState ? 1.f : -1.f);
    bool lastEasedIn = hotReloadStatusWindow.t > fullyEasedInThreshold;
    hotReloadStatusWindow.t = ImClamp(hotReloadStatusWindow.t + delta, 0.f, 1.f);
    bool easedIn = hotReloadStatusWindow.t > fullyEasedInThreshold;

    if (!lastEasedIn && easedIn) {
        hotReloadStatusWindow.timeVisible = 0.f;
    }

    if (easedIn) {
        hotReloadStatusWindow.timeVisible += state->dt;
    }

    if (hotReloadStatusWindow.autoClose &&
        hotReloadStatusWindow.targetState &&
        hotReloadStatusWindow.timeVisible > 3.f) {
        hotReloadStatusWindow.targetState = false;
    }

    float tEased = easeOutBack(hotReloadStatusWindow.t);

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 size = viewport->Size;
    size.x *= .5f;
    size.y *= .3f;
    ImVec2 pos = ImVec2(size.x, 35.f + 300.f * (tEased - 1.f));
    ImGui::SetNextWindowPos(pos, 0, ImVec2(.5f, 0.f));
    //ImGui::SetNextWindowSizeConstraints(ImVec2(0.f, 0.f), ImVec2())

    static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::PushStyleColor(ImGuiCol_Border, (ImVec4)ImColor::HSV((hotReloadStatusWindow.success ? 90.f : 2.f) / 360.f, .83f, .63f, .8f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.f, 8.f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 4.f);
    ImGui::Begin("hotReloadStatusWindow", NULL, flags);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (ImGui::IsWindowHovered()) {
            hotReloadStatusWindow.targetState = false;
        }
    }

    ImGui::SeparatorText(hotReloadStatusWindow.success ? "HOT-Reload Succeeded!" : "HOT-RELOAD Failed!");

    const char* lineStart = hotReloadStatusWindow.msg;
    const char* p = lineStart;
    // TODO: omg, doing this each frames ... I feel my ancestors turning around in their graves ...
    while (*p != '\0') {
        bool startsWithMake = strncmp(p, "make[", 5) == 0;
        bool startsWithCompiler = strncmp(p, "g++ ", 4) == 0;

        if (strncmp(p, "./build/../", 11) == 0) {
            lineStart += 11;
            p += 11;
        }

        bool isError = false;
        bool isWarning = false;
        bool isNote = false;
        while (*p != '\n' && *p != '\0') {
            if (strncmp(p, "error:", 6) == 0) {
                isError = true;
            }
            if (strncmp(p, "warning:", 8) == 0) {
                isWarning = true;
            }
            if (strncmp(p, "note:", 5) == 0) {
                isNote = true;
            }
            ++p;
        }
        if (!startsWithMake && !startsWithCompiler) {
            int lineLength = (int)(p - lineStart);
            if (isWarning || isError || isNote) {
                // TODO: kind hacky, but works :shrug:
                hotReloadStatusWindow.autoClose = false;
                float h = isError ? 4.f : (isWarning ? 55.f : 88.f);
                ImVec4 col = (ImVec4)ImColor::HSV(h / 360.f, .83f, .83f);
                ImGui::TextColored(col, "%*.*s", lineLength, lineLength, lineStart);
            }
            else {
                ImGui::Text("%*.*s", lineLength, lineLength, lineStart);
            }
        }
        if (*p == '\0') {
            break;
        }
        lineStart = p + 1;
        p = lineStart;
    }

    ImGui::End();
    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor();
}
#endif

static int runCmd(const char* cmd) {
    printf("[BUSCHLA] Running cmd '%s' ...", cmd);
    int code = system(cmd);
    printf(" returned %d\n", code);
    return code;
}

int main(int argc, char** argv) {
    Chars chars;
    memset(&chars, 0, sizeof(Chars));

    char strBuffer[PATH_MAX];

    AppState state;
    state.stateMemory = NULL;
    state.stateMemorySize = 0;
    bool exePathRet = getExecutablePath(argv[0], strBuffer);
    assert(exePathRet);
    splitAtLastOccurence(strBuffer, '/');

    // NOTE: All this string manipulation with the paths makes me nervous.
    // Especially because for hot-reloading we run rm commands.
    // So lets just plop an assert here to make sure our buffers are never overrun
    {
        size_t exePathLen = strlen(strBuffer);
        // Worst case is us running cp %s %s.
        assert(exePathLen < (PATH_MAX / 3));
    }

    state.exePath = ca_commit(&chars, strBuffer);
    printf("[BUSCHLA] exePath: '%s'\n", state.exePath);

#ifdef ENABLE_HOT_RELOADING
    DirWatcherState watcherState;
    memset(&watcherState, 0, sizeof(DirWatcherState));

    // NOTE: the exe is located in the build subfolder.
    const char* watchPaths[] = {
        "..",
        "../imgui",
    };
    char watcherPath[PATH_MAX];
    for (size_t i = 0; i < ARRAY_SIZE(watchPaths); ++i) {
        concatPaths(watcherPath, state.exePath, watchPaths[i]);
        DirWatcherConfig_AppendDirectory(&watcherState.config, watcherPath);
    }

    // TODO: configure reaction

    DirWatcher_Init(&watcherState);
#endif

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        printf("Error: SDL_Init(): %s\n", SDL_GetError());
        return 1;
    }

    float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
    SDL_WindowFlags window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL3+SDL_GPU example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
    if (window == nullptr) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    SDL_ShowWindow(window);

    SDL_GPUDevice* gpu_device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_METALLIB, true, nullptr);
    if (gpu_device == nullptr) {
        printf("Error: SDL_CreateGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        printf("Error: SDL_ClaimWindowForGPUDevice(): %s\n", SDL_GetError());
        return 1;
    }

    SDL_SetGPUSwapchainParameters(gpu_device, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

    IMGUI_CHECKVERSION();
    state.context = ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    main_scale *= 1.1f;
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    style.WindowBorderSize = 0.f;
    style.Colors[ImGuiCol_Text] = ImVec4(.8f, .8f, .8f, 1.f);

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                     // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // TODO: remove concatPaths and make a nice wrapper for snprintf and static buffer!
    //concatPaths(strBuffer, state.exePath, "/font.ttf");
    snprintf(strBuffer, sizeof(strBuffer), "%s/font.ttf", state.exePath);
    ImFont* font = io.Fonts->AddFontFromFileTTF(strBuffer);
    IM_ASSERT(font != NULL);

#ifdef ENABLE_HOT_RELOADING

    concatPaths(strBuffer, state.exePath, "/tmp");
    const char* tmpPath = ca_commit(&chars, strBuffer);
    printf("[BUSCHLA] tmpPath: '%s'\n", tmpPath);

    // TODO: This is kinda dangerous. if the path is cut off, it could be that we remove something unrelated!
    snprintf(strBuffer, sizeof(strBuffer), "rm -rf %s", tmpPath);
    runCmd(strBuffer);
    snprintf(strBuffer, sizeof(strBuffer), "mkdir %s", tmpPath);
    runCmd(strBuffer);

    concatPaths(strBuffer, state.exePath, "app.so");
    const char* appLibraryPath = ca_commit(&chars, strBuffer);
    printf("[BUSCHLA] appLibraryPath: '%s'\n", appLibraryPath);

    typedef struct {
        const char* tmpPath;
        const char* cmdCopyToTmp;
        const char* cmdRemoveTmp;
    } HotReloadData;

    HotReloadData hotReloadData[2];
    for (int i = 0; i < 2; ++i) {
        snprintf(strBuffer, sizeof(strBuffer), "%s/tmp%d", tmpPath, i);
        hotReloadData[i].tmpPath = ca_commit(&chars, strBuffer);

        snprintf(strBuffer, sizeof(strBuffer), "cp %s %s", appLibraryPath, hotReloadData[i].tmpPath);
        hotReloadData[i].cmdCopyToTmp = ca_commit(&chars, strBuffer);
    }

    int currentTmpLibraryIndex = 0;
    runCmd(hotReloadData[currentTmpLibraryIndex].cmdCopyToTmp);

    AppLibraryState appLibrary;
    if (!tryLoadAppLibrary(hotReloadData[currentTmpLibraryIndex].tmpPath, &appLibrary)) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
#endif

    ca_dump(stdout, &chars);

    float lastTime = ImGui::GetTime();
    bool done = false;
    while (!done) {
        state.time = ImGui::GetTime();
        state.dt = state.time - lastTime;
        lastTime = state.time;

        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT)
                done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

#ifdef ENABLE_HOT_RELOADING
        // Poll directory watcher events
        DirWatcherEvent watcherEvent;
        while (DirWatcher_PollEvent(&watcherState, &watcherEvent)) {
            if (watcherEvent.type == DIRWATCHER_EVENT_FILE_CHANGED) {
                for (int i = 0; i < watcherEvent.fileChanged.count; ++i) {
                    printf("[BUSCHLA] file changed: %s\n", watcherEvent.fileChanged.paths[i]);
                }
            }

            // TODO: BLÄH, this could be cleaner, right?
            if (watcherEvent.type == DIRWATCHER_EVENT_REACTION_REPORT) {
                printf("[BUSCHLA] reaction finished with code %d and msg:\n%s\n",
                    watcherEvent.reactionReport.exitStatus,
                    watcherEvent.reactionReport.statusMsg);

                if (watcherEvent.reactionReport.exitStatus == 0) {
                    int nextTmpLibraryIndex = (currentTmpLibraryIndex + 1) % 2;

                    int copyCmdStatus = runCmd(hotReloadData[nextTmpLibraryIndex].cmdCopyToTmp);
                    if (copyCmdStatus == 0) {
                        AppLibraryState newAppLibrary;
                        if (tryLoadAppLibrary(hotReloadData[nextTmpLibraryIndex].tmpPath, &newAppLibrary)) {
                            unloadAppLibrary(&appLibrary);

                            appLibrary.handle = newAppLibrary.handle;
                            appLibrary.app_main = newAppLibrary.app_main;
                            currentTmpLibraryIndex = nextTmpLibraryIndex;

                            printf("%s[BUSCHLA] Successfully replaced app library!%s\n", TERM_COL_GREEN_BOLD, TERM_COL_CLEAR);
                            triggerHotReloadStatus(true, watcherEvent.reactionReport.statusMsg);
                        }
                        else {
                            char* errorString = dlerror();
                            printf("%s[BUSCHLA] Error loading app library: %s%s\n", TERM_COL_RED_BOLD, errorString, TERM_COL_CLEAR);
                            triggerHotReloadStatus(false, errorString);
                        }
                    }
                    else {
                        const char* msg = "Unable to copy app.so to a tmp location!";
                        printf("%s[BUSCHLA] %s%s\n", TERM_COL_RED_BOLD, msg, TERM_COL_CLEAR);
                        triggerHotReloadStatus(false, msg);
                    }
                }
                else {
                    printf("%s[BUSCHLA] Unable to reload app library!%s\n", TERM_COL_RED_BOLD, TERM_COL_CLEAR);
                    triggerHotReloadStatus(false, watcherEvent.reactionReport.statusMsg);
                }
            }
        }
#endif

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
            continue;
        }

        ImGui_ImplSDLGPU3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

#ifdef ENABLE_HOT_RELOADING
        if (appLibrary.app_main != NULL) {
            appLibrary.app_main(&state);
        }

        drawHotReloadStatusWindow(&state);

#else
        app_main(&state);
#endif

        //ImGui::PopFont();
        ImGui::Render();
        ImDrawData* draw_data = ImGui::GetDrawData();
        const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);

        SDL_GPUCommandBuffer* command_buffer = SDL_AcquireGPUCommandBuffer(gpu_device);

        SDL_GPUTexture* swapchain_texture;
        SDL_WaitAndAcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr);

        if (swapchain_texture != nullptr && !is_minimized) {
            // This is mandatory: call ImGui_ImplSDLGPU3_PrepareDrawData() to upload the vertex/index buffer!
            ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, command_buffer);

            SDL_GPUColorTargetInfo target_info = {};
            target_info.texture = swapchain_texture;
            target_info.clear_color = clear_color;
            target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            target_info.store_op = SDL_GPU_STOREOP_STORE;
            target_info.mip_level = 0;
            target_info.layer_or_depth_plane = 0;
            target_info.cycle = false;
            SDL_GPURenderPass* render_pass = SDL_BeginGPURenderPass(command_buffer, &target_info, 1, nullptr);

            ImGui_ImplSDLGPU3_RenderDrawData(draw_data, command_buffer, render_pass);

            SDL_EndGPURenderPass(render_pass);
        }

        SDL_SubmitGPUCommandBuffer(command_buffer);
    }

    if (state.stateMemory != NULL) {
        free(state.stateMemory);
    }

#ifdef ENABLE_HOT_RELOADING
    if (hotReloadStatusWindow.msg != NULL) {
        free(hotReloadStatusWindow.msg);
    }

    unloadAppLibrary(&appLibrary);

    DirWatcher_Shutdown(&watcherState);

    // TODO: DANGEROUS!
    snprintf(strBuffer, sizeof(strBuffer), "rm -rf %s", tmpPath);
    runCmd(strBuffer);
#endif

    SDL_WaitForGPUIdle(gpu_device);
    ImGui_ImplSDL3_Shutdown();
    ImGui_ImplSDLGPU3_Shutdown();
    ImGui::DestroyContext();

    SDL_ReleaseWindowFromGPUDevice(gpu_device, window);
    SDL_DestroyGPUDevice(gpu_device);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
