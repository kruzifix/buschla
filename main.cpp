// Skeleton taken from imgui sdl3_sdlgpu3 example

#include <dlfcn.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <SDL3/SDL.h>

#include "app_interface.h"
#include "directory_watcher.h"
#include "util.h"
#include "imgui/imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlgpu3.h"

#define ENABLE_HOT_RELOADING

// TODO: How can we also build this to statically link the app layer?
// because we need the code hot reloading really only for dev on linux
typedef void (app_main_func)(AppState& state);

static SDL_FColor clear_color{ .1f, .1f, .1f, 1.f };

#define SPLITTER_SIZE 10.f
#define SPLIT_MIN_CONTENT_SIZE 100.f

// TODO: RIGHT CLICK => reset split!
void splitter(const char* label, float& splitValue, bool horizontal) {
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
        splitValue = SDL_clamp(splitValue + ImGui::GetIO().MouseDelta[axisIndex], SPLIT_MIN_CONTENT_SIZE, windowSize[axisIndex] - SPLIT_MIN_CONTENT_SIZE);
    }
}

void gui() {
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

int main(int argc, char** argv) {
#ifdef ENABLE_HOT_RELOADING
    char exePath[PATH_MAX];
    bool exePathRet = getExecutablePath(argv[0], exePath);
    assert(exePathRet);

    splitAtLastOccurence(exePath, '/');

    DirWatcherState watcherState;
    memset(&watcherState, 0, sizeof(DirWatcherState));

    // NOTE: the exe is located in the build subfolder.
    const char* watchPaths[] = {
        "..",
        "../imgui",
    };
    char watcherPath[PATH_MAX];
    for (int i = 0; i < ARRAY_SIZE(watchPaths); ++i) {
        concatPaths(watcherPath, exePath, watchPaths[i]);
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
    ImGui::CreateContext();
    // ImGuiIO &io = ImGui::GetIO();
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    //  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale); // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale; // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

    ImGui_ImplSDL3_InitForSDLGPU(window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {};
    init_info.Device = gpu_device;
    init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu_device, window);
    init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                     // Only used in multi-viewports mode.
    init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR; // Only used in multi-viewports mode.
    init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
    ImGui_ImplSDLGPU3_Init(&init_info);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details. If you like the default font but want it to scale better, consider using the 'ProggyVector' from the same author!
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    // style.FontSizeBase = 20.0f;
    // io.Fonts->AddFontDefault();
    // io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    // IM_ASSERT(font != nullptr);


    // TODO: We probably want the following prodcedure:
    // 1. Copy app.so to a temp file
    // 2. Then load&use that
    // Because when we rebuild app.so, it can't be open by the main proc!
    // ====> Maybe this is only a windows problem, we seem to be able to write to app.so, while we have it open!

    // TODO: build absolute path to library! (else you have to start the app from its directory which is stupid)
    // OR: we also try to open it in the build sub folder and use whatever is available
    const char* appLibraryPath = "./build/app.so";
    void* appHandle = dlopen(appLibraryPath, RTLD_NOW);
    if (appHandle == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }

    app_main_func* app_main = NULL;
    if (appHandle != NULL) {
        app_main = (app_main_func*)dlsym(appHandle, "app_main");
        if (app_main == NULL) {
            fprintf(stderr, "%s\n", dlerror());
            exit(2);
        }
    }

    AppState state;
    state.context = ImGui::GetCurrentContext();

    bool done = false;
    while (!done) {
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
        //printf("checking dir watcher. pipeClosed? %d\n", watcherState.pipeClosed);
        while (DirWatcher_PollEvent(&watcherState, &watcherEvent)) {
            if (watcherEvent.type == DIRWATCHER_EVENT_FILE_CHANGED) {
                for (int i = 0; i < watcherEvent.fileChanged.count; ++i) {
                    printf("[BUSCHLA] file changed: %s\n", watcherEvent.fileChanged.paths[i]);
                }
            }
            if (watcherEvent.type == DIRWATCHER_EVENT_REACTION_REPORT) {
                printf("[BUSCHLA] reaction finished with code %d and msg:\n%s\n",
                    watcherEvent.reactionReport.exitStatus,
                    watcherEvent.reactionReport.statusMsg);
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

        if (app_main != NULL) {
            //gui();
            app_main(state);
        }

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

#ifdef ENABLE_HOT_RELOADING
    DirWatcher_Shutdown(&watcherState);
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
