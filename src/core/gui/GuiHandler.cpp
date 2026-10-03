#include "GuiHandler.h"

#include "shader/CompileShaderWindow.h"

#include <format>
#include <array>
#include <filesystem>

#include "core/logger/SdlLoggerHandler.h"

#include <SDL3/SDL.h>
#include <imgui/backends/imgui_impl_sdl3.h>

#include "imgui_internal.h"
#include "builder/GuiBuilder.h"
#include "imgui/backends/imgui_impl_sdlrenderer3.h"

namespace {
    using Logger = engine::core::logger::SdlLoggerHandler;

    constexpr auto *kEngineGuiRenderName{"EngineGuiRenderer"};
    constexpr auto kMainScale{1.0f};
    constexpr auto kClearColor{ImVec4(0.45f, 0.55f, 0.60f, 1.00f)};

    void statGuiFrame() {
        // Start the Dear ImGui frame
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
    }

    void renderMainWindow(SDL_Renderer *renderer, GuiWindow *window) {
        const ImGuiIO &io = ImGui::GetIO();
        (void) io;

        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
            SDL_Delay(10);
        }

        statGuiFrame();

        if (ImGui::BeginMainMenuBar()) {
            engine::core::gui::addMenu<3>("File", {
                                              engine::core::gui::MenuItem{
                                                  .label = "Open", .shortcut = "Ctrl+O",
                                              },
                                              engine::core::gui::MenuItem{
                                                  .label = "Save", .shortcut = "Ctrl+S",
                                              },
                                              engine::core::gui::MenuItem{
                                                  .label = "Close", .shortcut = "Ctrl+W"
                                              }
                                          });
            engine::core::gui::addMenu<2>("Shaders", {
                                              engine::core::gui::MenuItem{
                                                  .label = "Open SPIRV", .shortcut = "",
                                                  .onItemClicked = [] { Logger::debug("Open SPIRV ..."); }
                                              },
                                              engine::core::gui::MenuItem{
                                                  .label = "Compile GLSL", .shortcut = "",
                                                  .onItemClicked = [] {
                                                      Logger::debug("GLSL Compile clicked ...");
                                                      engine::core::gui::setShowCompileShaderWindow(true);
                                                  }
                                              },
                                          });
            ImGui::EndMainMenuBar();
        }

        if (engine::core::gui::showCompileShaderWindow()) {
            engine::core::gui::compileShaderWindow();
        }

        if (engine::core::gui::showPersistShaderWindow()) {
            engine::core::gui::persistShaderWindow();
        }

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
    GuiHandler::~GuiHandler() {
        Logger::debug("Destroying gui resources");

        // Cleanup
        // [If using SDL_MAIN_USE_CALLBACKS: all code below would likely be your SDL_AppQuit() function]
        ImGui_ImplSDLRenderer3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();

        SDL_DestroyRenderer(renderer);
        logger::ILogger::debug("Gui resources cleaned");
    }

    void GuiHandler::init(SDL_Window *window) {
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

    void GuiHandler::renderGui(GuiWindow *window) const {
        renderMainWindow(renderer, window);
    }

    void GuiHandler::handleEvent(const GuiEvent &event) const {
        ImGui_ImplSDL3_ProcessEvent(&event);
    }
}
