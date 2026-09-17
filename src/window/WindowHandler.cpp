#include "WindowHandler.h"
#include "core/logger/SdlLoggerHandler.h"

#include <SDL3/SDL_video.h>

#include <format>
#include <utility>

namespace {
    SDL_Window *
    createWindowFromSettings(const engine::window::WindowHandler::WindowSettings &window_settings) noexcept {
        auto flags{0U};
        for (auto flag: window_settings.mFlags) {
            flags |= static_cast<uint32_t>(flag);
        }
        return SDL_CreateWindow(window_settings.mTitle.c_str(), window_settings.mWidth, window_settings.mHeight, flags);
    }
}

namespace engine::window {
    WindowHandler::WindowHandler(const WindowSettings &windowSettings)
        : _settingsWindow(std::make_shared<WindowSettings>(windowSettings)) {
        this->_main_window = createWindowFromSettings(*this->_settingsWindow);
        core::logger::SdlLoggerHandler::debug(
            std::format("Created window with id {}", SDL_GetWindowID(this->_main_window)));
    }

    WindowHandler::WindowSettings::WindowSettings(std::string title, const std::int32_t width,
                                                  const std::int32_t height,
                                                  std::list<WindowFlags> flags) : mTitle(std::move(title)),
        mWidth(width), mHeight(height), mFlags(std::move(flags)) {
    }

    WindowHandler::~WindowHandler() {
        core::logger::ILogger::debug("Destroying main window");
        if (!_windows.empty()) {
            for (auto *window: this->_windows) {
                SDL_DestroyWindow(window);
            }
        }
        SDL_DestroyWindow(this->_main_window);
    }

    std::uint32_t WindowHandler::createWindow(const WindowSettings &window_settings) noexcept {
        if (auto *window = createWindowFromSettings(window_settings)) {
            const auto id {SDL_GetWindowID(window)};
            core::logger::SdlLoggerHandler::debug(std::format("Created window with id {}", id));
            this->_windows.emplace_back(window);
            return id;
        }
        core::logger::SdlLoggerHandler::error(std::format("Failed to create window with id {}", SDL_GetError()));
        return -1;
    }

    void WindowHandler::destroyWindow(const std::uint32_t windowId) noexcept {
        if (this->_windows.empty() && windowId == SDL_GetWindowID(this->_main_window)) {
            core::logger::ILogger::debug("Destroying main window");
            SDL_GetWindowID(this->_main_window);
            return;
        }

        Window* windowToDestroy = nullptr;
        for (auto *window: this->_windows) {
            if (windowId == SDL_GetWindowID(window)) {
                windowToDestroy = window;
            }
        }

        if (windowToDestroy) {
            this->_windows.remove(windowToDestroy);
            SDL_DestroyWindow(windowToDestroy);
        }
    }

    SDL_Window *WindowHandler::getMainWindow() const {
        return this->_main_window;
    }
} // namespace engine::window
