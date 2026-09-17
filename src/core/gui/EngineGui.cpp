#include "EngineGui.h"

#include <format>
#include <queue>

#include "core/logger/SdlLoggerHandler.h"

#include <SDL3/SDL.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include "imgui/backends/imgui_impl_sdlrenderer3.h"
#include <imgui/imgui.h>

namespace {
    constexpr auto *kEngineGuiRenderName{"EngineGuiRenderer"};
    const auto kMainScale{SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay()) ? 1.0f : 1.0f};
    constexpr auto kClearColor{ImVec4(0.45f, 0.55f, 0.60f, 1.00f)};
    bool show_demo_window{false};

    void render(SDL_Renderer *renderer, GuiWindow *window) {
        const ImGuiIO &io = ImGui::GetIO();
        (void) io;

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
        }

        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        ImGui::BeginMainMenuBar();
        ImGui::Checkbox("Demo Window", &show_demo_window);
        ImGui::EndMainMenuBar();

        // Rendering
        ImGui::Render();
        SDL_SetRenderScale(renderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
        SDL_SetRenderDrawColorFloat(renderer, kClearColor.x, kClearColor.y, kClearColor.z, kClearColor.w);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }
}

namespace engine::core::gui {
    EngineGui::~EngineGui() {
        logger::ILogger::debug("Destroying gui resources");

        // Cleanup
        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        logger::ILogger::debug("Gui resources cleaned");
    }

    void EngineGui::init(SDL_Window *window) {
        logger::ILogger::debug("Setting up renderer ...");
        if (window == nullptr) {
            throw std::runtime_error("ERROR: Windows is null");
        }

        renderer = SDL_CreateRenderer(window, nullptr);
        SDL_SetRenderVSync(renderer, 1);
        if (renderer == nullptr) {
            throw std::runtime_error(std::format("ERROR: SDL_CreateRenderer(): {}", SDL_GetError()));
        }

        SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        SDL_ShowWindow(window);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup scaling
        ImGuiStyle &style = ImGui::GetStyle();
        style.ScaleAllSizes(kMainScale);
        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
        style.FontScaleDpi = kMainScale;
        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

        // Setup Platform/Renderer backends
        ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
        ImGui_ImplSDLRenderer3_Init(renderer);
    }

    void EngineGui::renderGui(GuiWindow *window) {
        render(renderer, window);
    }

    void EngineGui::handleEvent(const GuiEvent &event) const {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}
