#include "EngineHandler.h"

#include <atomic>

#include "core/gui/EngineGui.h"
#include "core/3d/TestTriangle.h"

#include <SDL3/SDL.h>

#include <format>

#include "core/logger/SdlLoggerHandler.h"

namespace {
    using namespace engine;

    std::atomic_bool running{true};
    std::atomic_bool initialized{false};
}

core::handler::EngineHandler::~EngineHandler() {
    logger::ILogger::info("SDL destroyed called");
    terminateAllResources();
}

void core::handler::EngineHandler::initialize(const std::list<ModuleType> &moduleTypes) noexcept {
    auto modules{0u};
    for (auto moduleType: moduleTypes) {
        modules |= static_cast<std::uint32_t>(moduleType);
    }

    if (!SDL_Init(modules)) {
        logger::ILogger::error(std::format("SDL initialized failed: {}", SDL_GetError()));
        std::terminate();
    }

    if (!windowHandler) {
        windowHandler = std::make_unique<window::WindowHandler>(window::WindowHandler::WindowSettings{
            "TestWindow", 1280, 720,
            {
                window::WindowHandler::WindowFlags::WINDOW_RESIZABLE,
                window::WindowHandler::WindowFlags::WINDOW_HIGH_PIXEL_DENSITY
            }
        });
    }

    if (!gpuDeviceHandler) {
        gpuDeviceHandler = std::make_unique<graphics::GpuDeviceHandler>(true);
    }

    if (!engineGuiHandler) {
        engineGuiHandler = std::make_unique<gui::EngineGui>();
    }

    logger::ILogger::debug(std::format("GPU driver: {}", gpuDeviceHandler->getDeviceDriver()));

    if (!gpuDeviceHandler->claimWindow(windowHandler->getMainWindow())) {
        logger::ILogger::error(
            std::format("SDL_ClaimWindowForGPUDevice failed: {}", SDL_GetError()));
        std::terminate();
    }
    engineGuiHandler->init(windowHandler->getMainWindow());
    initialized.store(true);
    logger::ILogger::info("SDL initialized successfully");
}

void core::handler::EngineHandler::run(const int argc, const char **argv) const {
    if (!initialized.load()) {
        throw std::runtime_error("Engine initialized failed, please run EngineHandler::initialize() first");
    }
    static_cast<void>(argc);
    static_cast<void>(argv);

    running.store(true);
    logger::ILogger::info("Starting loop");
    while (running.load()) {
        engineGuiHandler->renderGui(windowHandler->getMainWindow());
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    logger::ILogger::debug("Quit requested");
                    running.store(false);
                    break;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    releaseAllResources();
                    windowHandler->destroyWindow(event.window.windowID);
                    break;
                default:
                    engineGuiHandler->handleEvent(event);
            }
        }
    }
}

const std::unique_ptr<graphics::GpuDeviceHandler> &core::handler::EngineHandler::getGpuDeviceHandler() noexcept {
    return gpuDeviceHandler;
}

const std::unique_ptr<core::gui::EngineGui> & core::handler::EngineHandler::getGuiHandler() noexcept {
    return engineGuiHandler;
}

const std::unique_ptr<window::WindowHandler> &core::handler::EngineHandler::getWindowHandler() noexcept {
    return windowHandler;
}

void core::handler::EngineHandler::createSecondaryWindow() noexcept {
}

void core::handler::EngineHandler::releaseAllResources() const {
    /* Always wait for the GPU to finish before destroying things */
    SDL_WaitForGPUIdle(gpuDeviceHandler->getDevice());

    /* Release resources in reverse creation order */
    // SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    // SDL_ReleaseGPUBuffer(device, vertex_buffer);
    // SDL_ReleaseGPUTexture(device, texture);
    // SDL_ReleaseGPUSampler(device, sampler);
    // SDL_ReleaseGPUShader(device, vert_shader);
    // SDL_ReleaseGPUShader(device, frag_shader);

    gpuDeviceHandler->releaseWindowFromGpuDevice(windowHandler->getMainWindow());
}

void core::handler::EngineHandler::terminateAllResources() {
    gpuDeviceHandler.reset();
    engineGuiHandler.reset();
    windowHandler.reset();
    SDL_Quit();
    logger::ILogger::info("SDL terminated successfully");
}
